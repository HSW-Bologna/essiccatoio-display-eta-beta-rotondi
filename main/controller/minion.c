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
#define MODBUS_REQUEST_MESSAGE_QUEUE_SIZE  8
#define MODBUS_RESPONSE_MESSAGE_QUEUE_SIZE 8
#define MODBUS_TIMEOUT                     40
#define MODBUS_MAX_PACKET_SIZE             256
#define MODBUS_COMMUNICATION_ATTEMPTS      3

#define MODBUS_IR_DEVICE_MODEL 0
#define MODBUS_IR_CYCLE_STATE  9

#define MODBUS_HR_COMMAND        0
#define MODBUS_HR_TEST_MODE      1
#define MODBUS_HR_PROGRAM_NUMBER 11

#define MINION_ADDR 1

#define COMMAND_REGISTER_NONE   0
#define COMMAND_REGISTER_RESUME 1
#define COMMAND_REGISTER_PAUSE  2
#define COMMAND_REGISTER_DONE   3


typedef enum {
    TASK_MESSAGE_TAG_SYNC,
    TASK_MESSAGE_TAG_HANDSHAKE,
    TASK_MESSAGE_TAG_COMMAND,
    TASK_MESSAGE_TAG_RETRY_COMMUNICATION,
} task_message_tag_t;


struct __attribute__((packed)) task_message {
    task_message_tag_t tag;

    union {
        struct {
            uint8_t  test_mode;
            uint16_t test_outputs;
            uint8_t  test_pwm1;
            uint8_t  test_pwm2;
            uint16_t cycle_delay_time;
            uint16_t flags;
            uint16_t rotation_running_time;
            uint16_t rotation_pause_time;
            uint16_t rotation_speed;
            uint16_t drying_duration;
            uint16_t drying_type;
            uint16_t program_number;
            uint16_t step_number;
        } sync;

        uint16_t command;
    } as;
};


typedef struct {
    uint16_t start;
    void    *pointer;
} master_context_t;


static void        minion_task(void *args);
uint8_t            handle_message(ModbusMaster *master, struct task_message message);
static ModbusError exception_callback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code);
static ModbusError data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args);
static int write_holding_registers(ModbusMaster *master, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num);
static int read_holding_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                  uint16_t count);
static int read_input_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                uint16_t count);


static const char   *TAG       = "Modbus";
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


void minion_resume_program(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_RESUME}};
    xQueueSend(messageq, &msg, 0);
}


void minion_pause_program(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_PAUSE}};
    xQueueSend(messageq, &msg, 0);
}


void minion_program_done(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_COMMAND, .as = {.command = COMMAND_REGISTER_DONE}};
    xQueueSend(messageq, &msg, 0);
}


void minion_handshake(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_HANDSHAKE};
    xQueueSend(messageq, &msg, 0);
}


void minion_sync(model_t *model) {
    program_drying_parameters_t drying_step = model_get_current_step(model);

    struct task_message msg = {
        .tag = TASK_MESSAGE_TAG_SYNC,
        .as =
            {
                .sync =
                    {
                        .test_mode             = model->run.minion.write.test_mode,
                        .test_outputs          = model->run.minion.write.test_outputs,
                        .test_pwm1             = model->run.minion.write.test_pwm1,
                        .test_pwm2             = model->run.minion.write.test_pwm2,
                        .cycle_delay_time      = model->config.parmac.tempo_attesa_partenza_ciclo,
                        .flags                 = ((model->config.parmac.stop_tempo_ciclo > 0) << 0),
                        .rotation_running_time = drying_step.rotation_time,
                        .rotation_pause_time   = drying_step.pause_time,
                        .rotation_speed        = drying_step.speed,
                        .drying_duration       = drying_step.duration,
                        .drying_type           = drying_step.type,
                        .program_number        = model->run.current_program_index,
                        .step_number           = model->run.current_step_index,
                    },
            },
    };
    xQueueSend(messageq, &msg, 0);
}


uint8_t minion_get_response(minion_response_t *response) {
    return xQueueReceive(responseq, response, 0);
}


static void minion_task(void *args) {
    (void)args;
    ModbusMaster    master;
    ModbusErrorInfo err = modbusMasterInit(&master,
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
                    communication_error = handle_message(&master, last_unsuccessful_message);
                    vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT / 2));
                }
            } else {
                last_unsuccessful_message = message;
                communication_error       = handle_message(&master, message);
                vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT / 2));
            }
        }
    }

    vTaskDelete(NULL);
}


uint8_t handle_message(ModbusMaster *master, struct task_message message) {
    uint8_t           error    = 0;
    minion_response_t response = {};

    switch (message.tag) {
        case TASK_MESSAGE_TAG_COMMAND: {
            uint16_t command = message.as.command;
            ESP_LOGI(TAG, "Sending command %i to minion", command);
            if (write_holding_registers(master, MINION_ADDR, MODBUS_HR_COMMAND, &command, 1)) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
                xQueueSend(responseq, &response, portMAX_DELAY);
            }
            break;
        }

        case TASK_MESSAGE_TAG_HANDSHAKE: {
            response.tag = MINION_RESPONSE_TAG_HANDSHAKE;

            uint16_t cycle_state = 0;
            if (read_input_registers(master, &cycle_state, MINION_ADDR, MODBUS_IR_CYCLE_STATE, 1)) {
                error = 1;
            } else {
                response.as.handshake.cycle_state = cycle_state;

                uint16_t values[2] = {0};

                if (read_holding_registers(master, values, MINION_ADDR, MODBUS_HR_PROGRAM_NUMBER,
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

            uint16_t values[14] = {0};
            if (read_input_registers(master, values, MINION_ADDR, MODBUS_IR_DEVICE_MODEL,
                                     sizeof(values) / sizeof(values[0]))) {
                error = 1;
            } else {
                response.as.sync.firmware_version_major = (values[1] >> 11) & 0x1F;
                response.as.sync.firmware_version_minor = (values[1] >> 6) & 0x1F;
                response.as.sync.firmware_version_patch = (values[1] >> 0) & 0x3F;
                response.as.sync.inputs                 = values[2];
                response.as.sync.temperature_1_adc      = values[3];
                response.as.sync.temperature_1          = values[4];
                response.as.sync.temperature_2_adc      = values[5];
                response.as.sync.temperature_2          = values[6];
                response.as.sync.pressure_adc           = values[7];
                response.as.sync.pressure               = values[8];
                response.as.sync.cycle_state            = values[9];
                response.as.sync.step_type              = values[10];
                response.as.sync.default_temperature    = values[11];
                response.as.sync.remaining_time_seconds = values[12];
                response.as.sync.alarms                 = values[13];
            }

            if (!error) {
                uint16_t values[12] = {
                    message.as.sync.test_mode,
                    message.as.sync.test_outputs,
                    message.as.sync.test_pwm1 | (message.as.sync.test_pwm2 << 8),
                    message.as.sync.cycle_delay_time,
                    message.as.sync.flags,
                    message.as.sync.rotation_running_time,
                    message.as.sync.rotation_pause_time,
                    message.as.sync.rotation_speed,
                    message.as.sync.drying_duration,
                    message.as.sync.drying_type,
                    message.as.sync.program_number,
                    message.as.sync.step_number,
                };

                if (write_holding_registers(master, MINION_ADDR, MODBUS_HR_TEST_MODE, values,
                                            sizeof(values) / sizeof(values[0]))) {
                    error = 1;
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


static ModbusError data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args) {
    master_context_t *ctx = modbusMasterGetUserPointer(master);

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


static ModbusError exception_callback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code) {
    (void)master;
    ESP_LOGI(TAG, "Received exception (function %d) from slave %d code %d", function, address, code);

    return MODBUS_OK;
}


static int write_holding_registers(ModbusMaster *master, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num) {
    uint8_t buffer[MODBUS_MAX_PACKET_SIZE] = {0};
    int     res                            = 0;
    size_t  counter                        = 0;

    bsp_rs232_flush();

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest16RTU(master, address, starting_address, num, data);
        assert(modbusIsOk(err));
        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
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


static int read_holding_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                  uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    bsp_rs232_flush();

    master_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(master, NULL);
    } else {
        modbusMasterSetUserPointer(master, &ctx);
    }

    do {
        res = 0;
        err = modbusBuildRequest03RTU(master, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Read holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int read_input_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    master_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(master, NULL);
    } else {
        modbusMasterSetUserPointer(master, &ctx);
    }

    bsp_rs232_flush();

    do {
        res = 0;
        err = modbusBuildRequest04RTU(master, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
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
