#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lightmodbus/lightmodbus.h"
#include "minion.h"
#include "esp_log.h"
#include "bsp/rs232.h"
#include "model/model.h"


#define MODBUS_RESPONSE_03_LEN(data_len)   (5 + data_len * 2)
#define MODBUS_RESPONSE_05_LEN             8
#define MODBUS_RESPONSE_16_LEN             8
#define MODBUS_REQUEST_MESSAGE_QUEUE_SIZE  8
#define MODBUS_RESPONSE_MESSAGE_QUEUE_SIZE 4
#define MODBUS_TIMEOUT                     40
#define MODBUS_MAX_PACKET_SIZE             256
#define MODBUS_COMMUNICATION_ATTEMPTS      3

#define MODBUS_IR_DEVICE_MODEL 0
#define MODBUS_IR_CYCLE_STATE  9

#define MODBUS_HR_TEST_MODE         0
#define MODBUS_HR_PROGRAM_NUMBER    24
#define MODBUS_HR_COMMAND           27
#define MODBUS_HR_INCREASE_DURATION 28
#define MODBUS_HR_SET_DURATION      29

#define MINION_ADDR 1

#define COMMAND_REGISTER_NONE                   0
#define COMMAND_REGISTER_RESUME                 1
#define COMMAND_REGISTER_PAUSE                  2
#define COMMAND_REGISTER_STANDBY                3
#define COMMAND_REGISTER_DONE                   4
#define COMMAND_REGISTER_CLEAR_ALARMS           5
#define COMMAND_REGISTER_CLEAR_COINS            6
#define COMMAND_REGISTER_CLEAR_CYCLE_STATISTICS 7


typedef enum {
    TASK_MESSAGE_TAG_SYNC,
    TASK_MESSAGE_TAG_HANDSHAKE,
    TASK_MESSAGE_TAG_COMMAND,
    TASK_MESSAGE_TAG_INCREASE_DURATION,
    TASK_MESSAGE_TAG_SET_DURATION,
    TASK_MESSAGE_TAG_CHANGE_STEP,
    TASK_MESSAGE_TAG_RETRY_COMMUNICATION,
} task_message_tag_t;


struct __attribute__((packed)) task_message {
    task_message_tag_t tag;

    union {
        struct {
            uint8_t  test_mode;
            uint8_t  busy_signal_type;
            uint8_t  test_pwm1;
            uint8_t  test_pwm2;
            uint16_t test_outputs;
            uint8_t  coin_reader_inhibition;
            uint16_t cycle_delay_time;
            uint16_t cycle_reset_time;
            uint16_t output_safety_temperature;
            uint16_t temperature_alarm_delay_seconds;
            uint16_t air_flow_alarm_time;
            uint16_t temperature_probe;
            uint16_t heating_type;
            uint16_t gas_ignition_attempts;
            uint16_t fan_with_open_porthole_time;
            uint16_t flags;
            uint16_t rotation_running_time;
            uint16_t rotation_pause_time;
            uint16_t rotation_speed;
            uint16_t duration;
            uint16_t setpoint_temperature;
            uint16_t setpoint_humidity;
            uint16_t temperature_cooling_hysteresis;
            uint16_t temperature_heating_hysteresis;
            uint16_t drying_type;
            uint16_t program_number;
            uint16_t step_number;
            uint16_t step_type;
            uint16_t command;
        } sync;

        uint16_t command;
        uint16_t duration;
    } as;
};


typedef struct {
    uint16_t start;
    void    *pointer;
} network_context_t;


static void        minion_task(void *args);
uint8_t            handle_message(ModbusMaster *network, struct task_message message);
static ModbusError exception_callback(const ModbusMaster *network, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code);
static ModbusError data_callback(const ModbusMaster *network, const ModbusDataCallbackArgs *args);
static int  write_holding_registers(ModbusMaster *network, uint8_t address, uint16_t starting_address, uint16_t *data,
                                    size_t num);
static int  read_holding_registers(ModbusMaster *network, uint16_t *registers, uint8_t address, uint16_t start,
                                   uint16_t count);
static int  read_input_registers(ModbusMaster *network, uint16_t *registers, uint8_t address, uint16_t start,
                                 uint16_t count);
static void sync_with_command(model_t *model, uint16_t command);
static struct task_message init_sync_data(model_t *model);
void                       init_sync_holding_register_array(uint16_t *values, struct task_message *msg);


static const char   *TAG       = __FILE_NAME__;
static QueueHandle_t messageq  = NULL;
static QueueHandle_t responseq = NULL;


void minion_init(void) {
    static StaticQueue_t static_queue1;
    static uint8_t       queue_buffer1[MODBUS_REQUEST_MESSAGE_QUEUE_SIZE * sizeof(struct task_message)] = {0};
    messageq = xQueueCreateStatic(sizeof(queue_buffer1) / sizeof(struct task_message), sizeof(struct task_message),
                                  queue_buffer1, &static_queue1);

    static StaticQueue_t static_queue2;
    static uint8_t       queue_buffer2[MODBUS_RESPONSE_MESSAGE_QUEUE_SIZE * sizeof(minion_response_t)] = {0};
    responseq = xQueueCreateStatic(sizeof(queue_buffer2) / sizeof(minion_response_t), sizeof(minion_response_t),
                                   queue_buffer2, &static_queue2);

    xTaskCreate(minion_task, TAG, 512 * 6, NULL, 5, NULL);
}


void minion_retry_communication(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_RETRY_COMMUNICATION};
    xQueueSend(messageq, &msg, 0);
}


void minion_increase_duration(uint16_t seconds) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_INCREASE_DURATION, .as = {.duration = seconds}};
    xQueueSend(messageq, &msg, 0);
}


void minion_set_duration(uint16_t seconds) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_SET_DURATION, .as = {.duration = seconds}};
    xQueueSend(messageq, &msg, 0);
}


void minion_change_step(model_t *model) {
    struct task_message msg = init_sync_data(model);
    msg.tag                 = TASK_MESSAGE_TAG_CHANGE_STEP;
    msg.as.sync.command     = COMMAND_REGISTER_RESUME;
    xQueueSend(messageq, &msg, 0);
}


void minion_clear_alarms(model_t *model) {
    sync_with_command(model, COMMAND_REGISTER_CLEAR_ALARMS);
}


void minion_resume_program(model_t *model, uint8_t clear_alarms) {
    if (clear_alarms) {
        struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_CLEAR_ALARMS}};
        xQueueSend(messageq, &msg, 0);
    }
    sync_with_command(model, COMMAND_REGISTER_RESUME);
}


void minion_pause_program(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_PAUSE}};
    xQueueSend(messageq, &msg, 0);
}


void minion_program_done(model_t *model) {
    sync_with_command(model, COMMAND_REGISTER_DONE);
}


void minion_clear_cycle_statistics(model_t *model) {
    sync_with_command(model, COMMAND_REGISTER_CLEAR_CYCLE_STATISTICS);
}


void minion_clear_coins(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_CLEAR_COINS}};
    xQueueSend(messageq, &msg, 0);
}


void minion_handshake(model_t *model) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_HANDSHAKE};
    xQueueSend(messageq, &msg, 0);
    minion_clear_alarms(model);
}


void minion_sync(model_t *model) {
    sync_with_command(model, COMMAND_REGISTER_NONE);
}


uint8_t minion_get_response(minion_response_t *response) {
    return xQueueReceive(responseq, response, 0);
}


static void sync_with_command(model_t *model, uint16_t command) {
    struct task_message msg = init_sync_data(model);
    msg.as.sync.command     = command;
    xQueueSend(messageq, &msg, 0);
}


static void minion_task(void *args) {
    (void)args;
    ModbusMaster    network;
    ModbusErrorInfo err = modbusMasterInit(&network,
                                           data_callback,              // Callback for handling incoming data
                                           exception_callback,         // Exception callback (optional)
                                           modbusDefaultAllocator,     // Memory allocator used to allocate request
                                           modbusMasterDefaultFunctions,        // Set of supported functions
                                           modbusMasterDefaultFunctionCount     // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusMasterInit() failed");
    struct task_message last_unsuccessful_message = {0};
    struct task_message message                   = {0};
    uint8_t             communication_error       = 0;

    ESP_LOGI(TAG, "Task starting");

    for (;;) {
        if (xQueueReceive(messageq, &message, pdMS_TO_TICKS(100))) {
            if (communication_error) {
                if (message.tag == TASK_MESSAGE_TAG_RETRY_COMMUNICATION) {
                    communication_error = handle_message(&network, last_unsuccessful_message);
                    vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT / 2));
                }
            } else {
                last_unsuccessful_message = message;
                communication_error       = handle_message(&network, message);
                vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT / 2));
            }
        }
    }

    vTaskDelete(NULL);
}


uint8_t handle_message(ModbusMaster *network, struct task_message message) {
    uint8_t           error    = 0;
    minion_response_t response = {};

    switch (message.tag) {
        case TASK_MESSAGE_TAG_COMMAND: {
            uint16_t command = message.as.command;
            ESP_LOGI(TAG, "Sending command %i", command);
            if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_COMMAND, &command, 1)) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
                xQueueSend(responseq, &response, portMAX_DELAY);
            }
            break;
        }

        case TASK_MESSAGE_TAG_CHANGE_STEP: {
            uint8_t  error   = 0;
            uint16_t command = COMMAND_REGISTER_STANDBY;

            if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_COMMAND, &command, 1)) {
                error = 1;
            } else {
                uint16_t values[28] = {0};
                init_sync_holding_register_array(values, &message);

                if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_TEST_MODE, values,
                                            sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                }
            }

            if (error) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
                xQueueSend(responseq, &response, portMAX_DELAY);
            }
            break;
        }

        case TASK_MESSAGE_TAG_INCREASE_DURATION: {
            uint16_t values[2] = {
                COMMAND_REGISTER_CLEAR_COINS,
                message.as.duration,
            };     // Clear credit and increase duration in one sweep

            if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_COMMAND, values,
                                        sizeof(values) / sizeof(values[0]))) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
                xQueueSend(responseq, &response, portMAX_DELAY);
            }
            break;
        }

        case TASK_MESSAGE_TAG_SET_DURATION: {
            uint16_t values[1] = {
                message.as.duration,
            };     // Clear credit and increase duration in one sweep

            if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_SET_DURATION, values,
                                        sizeof(values) / sizeof(values[0]))) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
                xQueueSend(responseq, &response, portMAX_DELAY);
            }
            break;
        }

        case TASK_MESSAGE_TAG_HANDSHAKE: {
            response.tag = MINION_RESPONSE_TAG_HANDSHAKE;

            uint16_t cycle_state = 0;
            if (read_input_registers(network, &cycle_state, MINION_ADDR, MODBUS_IR_CYCLE_STATE, 1)) {
                error = 1;
            } else {
                response.as.handshake.cycle_state = cycle_state;

                uint16_t values[2] = {0};

                if (read_holding_registers(network, values, MINION_ADDR, MODBUS_HR_PROGRAM_NUMBER,
                                           sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                } else {
                    response.as.handshake.program_index = values[0];
                    response.as.handshake.step_index    = values[1];
                }
            }

            if (error) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
            }

            xQueueSend(responseq, &response, portMAX_DELAY);
            break;
        }

        case TASK_MESSAGE_TAG_SYNC: {
            response.tag = MINION_RESPONSE_TAG_SYNC;

            {
                uint16_t values[28] = {0};
                init_sync_holding_register_array(values, &message);

                if (write_holding_registers(network, MINION_ADDR, MODBUS_HR_TEST_MODE, values,
                                            sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                }
            }

            if (!error) {
                uint16_t values[42] = {0};
                if (read_input_registers(network, values, MINION_ADDR, MODBUS_IR_DEVICE_MODEL,
                                         sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                } else {
                    response.as.sync.firmware_version_major   = (values[1] >> 11) & 0x1F;
                    response.as.sync.firmware_version_minor   = (values[1] >> 6) & 0x1F;
                    response.as.sync.firmware_version_patch   = (values[1] >> 0) & 0x3F;
                    response.as.sync.build_day                = (values[2] >> 8) & 0xFF;
                    response.as.sync.build_month              = values[2] & 0xFF;
                    response.as.sync.build_year               = values[3] & 0xFF;
                    response.as.sync.heating                  = (values[4] & 0x01) > 0;
                    response.as.sync.held_by_temperature      = (values[4] & 0x02) > 0;
                    response.as.sync.held_by_humidity         = (values[4] & 0x04) > 0;
                    response.as.sync.inputs                   = values[5];
                    response.as.sync.outputs                  = values[6];
                    response.as.sync.fan_speed                = values[7];
                    response.as.sync.drum_speed               = values[8];
                    response.as.sync.temperature_1_adc        = values[9];
                    response.as.sync.temperature_1            = values[10];
                    response.as.sync.temperature_2_adc        = values[11];
                    response.as.sync.temperature_2            = values[12];
                    response.as.sync.temperature_probe        = values[13];
                    response.as.sync.humidity_probe           = values[14];
                    response.as.sync.pressure_adc             = values[15];
                    response.as.sync.pressure                 = values[16];
                    response.as.sync.payment                  = values[17];
                    response.as.sync.coins[0]                 = values[18];
                    response.as.sync.coins[1]                 = values[19];
                    response.as.sync.coins[2]                 = values[20];
                    response.as.sync.coins[3]                 = values[21];
                    response.as.sync.coins[4]                 = values[22];
                    response.as.sync.cycle_state              = values[23];
                    response.as.sync.default_temperature      = values[24];
                    response.as.sync.elapsed_time_seconds     = values[25];
                    response.as.sync.remaining_time_seconds   = values[26];
                    response.as.sync.alarms                   = values[27];
                    response.as.sync.complete_cycles          = ((uint32_t)values[28] << 16) | ((uint32_t)values[29]);
                    response.as.sync.partial_cycles           = ((uint32_t)values[30] << 16) | ((uint32_t)values[31]);
                    response.as.sync.active_time_seconds      = ((uint32_t)values[32] << 16) | ((uint32_t)values[33]);
                    response.as.sync.work_time_seconds        = ((uint32_t)values[34] << 16) | ((uint32_t)values[35]);
                    response.as.sync.rotation_time_seconds    = ((uint32_t)values[36] << 16) | ((uint32_t)values[37]);
                    response.as.sync.ventilation_time_seconds = ((uint32_t)values[38] << 16) | ((uint32_t)values[39]);
                    response.as.sync.heating_time_seconds     = ((uint32_t)values[40] << 16) | ((uint32_t)values[41]);
                }
            }

            if (error) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
            }

            xQueueSend(responseq, &response, portMAX_DELAY);
            break;
        }

        case TASK_MESSAGE_TAG_RETRY_COMMUNICATION:
            break;
    }

    return error;
}


static ModbusError data_callback(const ModbusMaster *network, const ModbusDataCallbackArgs *args) {
    network_context_t *ctx = modbusMasterGetUserPointer(network);

    if (ctx != NULL) {
        switch (args->type) {
            case MODBUS_HOLDING_REGISTER: {
                uint16_t *buffer                 = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_DISCRETE_INPUT: {
                uint8_t *buffer                  = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_INPUT_REGISTER: {
                uint16_t *buffer                 = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_COIL: {
                uint8_t *buffer                  = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }
        }
    }

    return MODBUS_OK;
}


static ModbusError exception_callback(const ModbusMaster *network, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code) {
    (void)network;
    ESP_LOGI(TAG, "Received exception (function %d) from slave %d code %d", function, address, code);

    return MODBUS_OK;
}


static int write_holding_registers(ModbusMaster *network, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num) {
    uint8_t buffer[MODBUS_MAX_PACKET_SIZE] = {0};
    int     res                            = 0;
    size_t  counter                        = 0;

    bsp_rs232_flush();

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest16RTU(network, address, starting_address, num, data);
        assert(modbusIsOk(err));
        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(network), modbusMasterGetRequestLength(network));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err = modbusParseResponseRTU(network, modbusMasterGetRequest(network), modbusMasterGetRequestLength(network),
                                     buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Write holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        ESP_LOGW(TAG, "ERROR!");
    } else {
        ESP_LOGD(TAG, "Success");
    }

    return res;
}


static int read_holding_registers(ModbusMaster *network, uint16_t *registers, uint8_t address, uint16_t start,
                                  uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    bsp_rs232_flush();

    network_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(network, NULL);
    } else {
        modbusMasterSetUserPointer(network, &ctx);
    }

    do {
        res = 0;
        err = modbusBuildRequest03RTU(network, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(network), modbusMasterGetRequestLength(network));

        int len = bsp_rs232_read(buffer, MODBUS_RESPONSE_03_LEN(count));
        err = modbusParseResponseRTU(network, modbusMasterGetRequest(network), modbusMasterGetRequestLength(network),
                                     buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Read holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int read_input_registers(ModbusMaster *network, uint16_t *registers, uint8_t address, uint16_t start,
                                uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    network_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(network, NULL);
    } else {
        modbusMasterSetUserPointer(network, &ctx);
    }

    bsp_rs232_flush();

    do {
        res = 0;
        err = modbusBuildRequest04RTU(network, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(network), modbusMasterGetRequestLength(network));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err = modbusParseResponseRTU(network, modbusMasterGetRequest(network), modbusMasterGetRequestLength(network),
                                     buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Read input registers for %i error (%i): %i %i", address, len, err.source, err.error);
            ESP_LOG_BUFFER_HEX(TAG, buffer, len);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        ESP_LOGW(TAG, "ERROR!");
    } else {
        ESP_LOGD(TAG, "Success");
    }

    return res;
}


static struct task_message init_sync_data(model_t *model) {
    program_step_t step = model_get_current_step(model);

    struct task_message msg = {
        .tag = TASK_MESSAGE_TAG_SYNC,
        .as =
            {
                .sync =
                    {
                        .coin_reader_inhibition          = !model_should_enable_coin_reader(model),
                        .test_mode                       = model->run.minion.write.test_mode,
                        .test_outputs                    = model->run.minion.write.test_outputs,
                        .test_pwm1                       = model->run.minion.write.test_pwm1,
                        .test_pwm2                       = model->run.minion.write.test_pwm2,
                        .busy_signal_type                = model->config.parmac.tipo_macchina_occupata,
                        .cycle_delay_time                = model->config.parmac.tempo_attesa_partenza_ciclo,
                        .cycle_reset_time                = model->config.parmac.cycle_reset_time,
                        .output_safety_temperature       = model->config.parmac.safety_output_temperature,
                        .temperature_alarm_delay_seconds = model->config.parmac.tempo_allarme_temperatura,
                        .air_flow_alarm_time             = model->config.parmac.air_flow_alarm_time,
                        .flags                           = ((model->config.parmac.stop_time_in_pause > 0) << 0 |
                                  (model->config.parmac.disabilita_allarmi > 0) << 1 |
                                  (model->config.parmac.emergency_alarm_nc_na > 0) << 2 |
                                  (model->config.parmac.allarme_inverter_off_on > 0) << 3 |
                                  (model->config.parmac.allarme_filtro_off_on > 0) << 4 |
                                  (model->config.parmac.porthole_nc_na > 0) << 5 |
                                  (model->config.parmac.busy_signal_nc_na > 0) << 6),
                        .temperature_probe               = model->config.parmac.temperature_probe,
                        .heating_type                    = model->config.parmac.heating_type,
                        .gas_ignition_attempts           = model->config.parmac.gas_ignition_attempts,
                        .fan_with_open_porthole_time     = model->config.parmac.fan_with_open_porthole_time,
                        .program_number                  = model->run.current_program_index,
                        .setpoint_temperature            = model_get_temperature_setpoint(model),
                        .setpoint_humidity               = model_get_humidity_setpoint(model),
                        .rotation_speed                  = model_get_speed(model),
                        .step_number                     = model->run.current_step_index,
                        .step_type                       = step.type,
                    },
            },
    };

    switch (step.type) {
        case PROGRAM_STEP_TYPE_DRYING: {
            msg.as.sync.rotation_running_time          = step.drying.rotation_time;
            msg.as.sync.rotation_pause_time            = step.drying.pause_time;
            msg.as.sync.duration                       = model->config.parmac.payment_type == PAYMENT_TYPE_NONE
                                                             ? step.drying.duration * 60
                                                             : model_get_time_for_credit(model);
            msg.as.sync.drying_type                    = step.drying.type;
            msg.as.sync.temperature_cooling_hysteresis = step.drying.cooling_hysteresis;
            msg.as.sync.temperature_heating_hysteresis = step.drying.heating_hysteresis;

            msg.as.sync.flags |=
                ((step.drying.enable_reverse > 0) << 7) | ((step.drying.enable_waiting_for_temperature << 8));
            break;
        }

        case PROGRAM_STEP_TYPE_COOLING: {
            msg.as.sync.rotation_running_time = step.cooling.rotation_time;
            msg.as.sync.rotation_pause_time   = step.cooling.pause_time;
            msg.as.sync.duration              = step.cooling.duration * 60;

            msg.as.sync.flags |= ((step.cooling.enable_reverse > 0) << 7);
            break;
        }

        case PROGRAM_STEP_TYPE_ANTIFOLD: {
            msg.as.sync.duration = step.antifold.max_duration == 0 ? 0xFFFF : step.antifold.max_duration * 60;
            msg.as.sync.rotation_running_time = step.antifold.rotation_time;
            msg.as.sync.rotation_pause_time   = step.antifold.pause_time;
            msg.as.sync.flags |= (1 << 7);     // Always enable reverse in antifold
            break;
        }
    }

    return msg;
}


void init_sync_holding_register_array(uint16_t *values, struct task_message *msg) {
    values[0]  = msg->as.sync.test_mode;
    values[1]  = msg->as.sync.test_outputs;
    values[2]  = msg->as.sync.test_pwm1 | (msg->as.sync.test_pwm2 << 8);
    values[3]  = msg->as.sync.coin_reader_inhibition;
    values[4]  = msg->as.sync.busy_signal_type;
    values[5]  = msg->as.sync.output_safety_temperature;
    values[6]  = msg->as.sync.temperature_alarm_delay_seconds;
    values[7]  = msg->as.sync.air_flow_alarm_time;
    values[8]  = msg->as.sync.temperature_probe;
    values[9]  = msg->as.sync.heating_type;
    values[10] = msg->as.sync.gas_ignition_attempts;
    values[11] = msg->as.sync.fan_with_open_porthole_time;
    values[12] = msg->as.sync.cycle_delay_time;
    values[13] = msg->as.sync.cycle_reset_time;
    values[14] = msg->as.sync.flags;
    values[15] = msg->as.sync.duration;
    values[16] = msg->as.sync.rotation_running_time;
    values[17] = msg->as.sync.rotation_pause_time;
    values[18] = msg->as.sync.rotation_speed;
    values[19] = msg->as.sync.setpoint_temperature;
    values[20] = msg->as.sync.setpoint_humidity;
    values[21] = msg->as.sync.temperature_cooling_hysteresis;
    values[22] = msg->as.sync.temperature_heating_hysteresis;
    values[23] = msg->as.sync.drying_type;
    values[24] = msg->as.sync.program_number;
    values[25] = msg->as.sync.step_number;
    values[26] = msg->as.sync.step_type;
    values[27] = msg->as.sync.command;
}
