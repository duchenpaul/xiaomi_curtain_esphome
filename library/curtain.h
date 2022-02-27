#include "esphome.h"

// https://github.com/iphong/esphome-tuya-curtain/blob/master/curtain.h

// basic commands
#define BABAI_CLOSE "down set_properties 2 1 2"
#define BABAI_STOP "down set_properties 2 1 0"
#define BABAI_OPEN "down set_properties 2 1 1"

// set position percentage
#define BABAI_SET_POSITION "down set_properties 2 3 "
#define BABAI_GET_POSITION "down get_properties 2 3"
#define BABAI_POSITION_RESPOND "result 2 3 0 "
#define BABAI_POSITION_CHANGED "properties_changed"

// enable/disable reversed motor direction
#define BABAI_DISABLE_REVERSING "down set_properties 2 4 0"
#define BABAI_ENABLE_REVERSING "down set_properties 2 4 1"

#define BABAI_CALIBRATE "down set_properties 2 4 2"

#define RETRIES 1

// 0~100
static int position_recorded;
static int position_target;
static int position_current_trig = 0;

class CustomCurtain : public Component, public UARTDevice, public Cover
{
public:
    CustomCurtain(UARTComponent *parent) : UARTDevice(parent) {}
    CoverTraits get_traits() override
    {
        auto traits = CoverTraits();
        traits.set_is_assumed_state(false);
        traits.set_supports_position(true);
        traits.set_supports_tilt(false);
        return traits;
    }

    void setup() override
    {
        ESP_LOGD("cover", "<DEBUG> Initial Calibrate");
        writeSerial(BABAI_CALIBRATE);
    }

    void loop() override
    {
        while (available())
        {
            readSerial();
        }
    }

    void control(const CoverCall &call) override
    {
        if (call.get_stop())
        {
            writeSerial(BABAI_STOP);
            current_operation = COVER_OPERATION_IDLE;
        }
        if (call.get_position().has_value())
        {
            position_target = *call.get_position() * 100.0f;
            if (position_target == 100)
            {
                writeSerial(BABAI_OPEN);
            }
            else if (position_target == 0)
            {
                writeSerial(BABAI_CLOSE);
            }
            else
            {
                move_to_position(position_target);
            }

            ESP_LOGD("cover", "<DEBUG> change position from %d to %d", position_recorded, position_target);
        }
    }

    void move_to_position(int position_target)
    {
        ESP_LOGD("cover", "<DEBUG> move to %d", position_target);
        char buffer[300];
        sprintf(buffer, BABAI_SET_POSITION "%d", position_target);
        writeSerial(buffer);
    }

    int readline(int readch, char *buffer, int len)
    {
        static int pos = 0;
        int rpos;

        if (readch > 0)
        {
            switch (readch)
            {
            case '\n': // Ignore new-lines
                break;
            case '\r': // Return on CR
                rpos = pos;
                pos = 0; // Reset position index ready for next time
                return rpos;
            default:
                if (pos < len - 1)
                {
                    buffer[pos++] = readch;
                    buffer[pos] = 0;
                }
            }
        }
        // No end of line has been found, so return -1.
        return -1;
    }

    void readSerial()
    {
        const int max_line_length = 80;
        static char buffer[max_line_length];

        if (readline(read(), buffer, max_line_length) > 0)
        {
            std::string buffers = buffer;

            ESP_LOGD("cover", "<DEBUG> Recieved string: %s", buffers.c_str());

            if (strcmp(buffer, "get_down") == 0)
            {
                if (position_current_trig % 5 == 0)
                {
                    writeSerial(BABAI_GET_POSITION);
                }
                position_current_trig++;
                ESP_LOGD("cover", "<DEBUG> %d / %d", position_current_trig, position_current_trig % 5);
            }
            if (buffers.find(BABAI_POSITION_RESPOND) == 0)
            {
                char *end;
                this->position = atof(buffers.substr(strlen(BABAI_POSITION_RESPOND)).c_str()) / 100.0f;
                position_recorded = this->position * 100.0f;
                ESP_LOGD("cover", "<DEBUG> position: %d, target: %d", position_recorded, position_target);
                if (this->position == 0)
                {
                    this->position = COVER_CLOSED;
                }
                else if (this->position == 1)
                {
                    this->position = COVER_OPEN;
                }

                // update the operation status of cover
                if (position_target < position_recorded)
                {

                    ESP_LOGD("cover", "<DEBUG> Close to %d", position_target);
                    move_to_position(position_target);
                    current_operation = COVER_OPERATION_CLOSING;
                }
                else if (position_target > position_recorded)
                {

                    ESP_LOGD("cover", "<DEBUG> Open to %d", position_target);
                    move_to_position(position_target);
                    current_operation = COVER_OPERATION_OPENING;
                }
                else
                {
                    current_operation = COVER_OPERATION_IDLE;
                }
            }
            if (buffers.find(BABAI_POSITION_CHANGED) == 0)
            {
                current_operation = COVER_OPERATION_IDLE;
                writeSerial(BABAI_GET_POSITION);
            }
            this->publish_state();
        }
    }

    void writeSerial(const char *str)
    {
        char buf[128];
        sprintf(buf, "%s\r", str);
        for (int i = 0; i < RETRIES; i++)
        {
            write_str(buf);
        }
    }
};

class CustomAPI : public Component,
                  public CustomAPIDevice,
                  public UARTDevice
{
public:
    CustomAPI(UARTComponent *parent) : UARTDevice(parent) {}
    void setup() override
    {
        register_service(&CustomAPI::setMotorNormal, "set_motor_normal");
        register_service(&CustomAPI::setMotorReversed, "set_motor_reversed");
        register_service(&CustomAPI::sendCalibrate, "send_calibrate");
        register_service(&CustomAPI::sendMessage, "send_command", {"data"});
    }

    void sendCalibrate()
    {
        ESP_LOGD("cover", "<DEBUG> Calibrate");
        writeSerial(BABAI_CALIBRATE);
    }

    void setMotorNormal()
    {
        writeSerial(BABAI_DISABLE_REVERSING);
        id(cover_reversed) = true;
    }

    void setMotorReversed()
    {
        writeSerial(BABAI_ENABLE_REVERSING);
        id(cover_reversed) = false;
    }

    void sendMessage(std::string data)
    {
        char buffer[50];
        strcpy(buffer, data.c_str());
        writeSerial(buffer);
    }

    void writeSerial(const char *str)
    {
        char buf[128];
        sprintf(buf, "%s\r", str);
        for (int i = 0; i < RETRIES; i++)
        {
            write_str(buf);
        }
    }
};
