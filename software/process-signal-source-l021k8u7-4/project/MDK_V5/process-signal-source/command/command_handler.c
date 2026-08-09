#include "command_handler.h"
#include "command_table.h"
#include "global_vars.h"
#include "user.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static char resp[1024];

char* lscmd_handler(int argc, char **argv)
{
    // 帮助信息
    if (argc == 2 && 
       (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        sprintf(resp,
            "Usage: lscmd [options]\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "List all supported commands.\r\n");
        return resp;
    }
    
    if (argc == 1)
    {
        // 默认行为：列出所有命令
        int count = command_table_count();
        int offset = snprintf(resp, sizeof(resp), "Supported commands:\r\n");
        for (int i = 0; i < count && offset < sizeof(resp); i++)
        {
            const char* name = command_table_get_name(i);
            if (name)
            {
                offset += snprintf(resp + offset, sizeof(resp) - offset, "  %s\r\n", name);
            }
        }
        return resp;
    }

    // 无效参数
    sprintf(resp, "Invalid option. Try 'lscmd -h'\r\n");
    return resp;
}

char* echo_handler(int argc, char **argv)
{
    // 帮助信息
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
                 "Usage: echo [text]\r\n"
                 "Options:\r\n"
                 "  -h, --help    Show this help message\r\n"
                 "\r\n"
                 "Print the given text to output.\r\n");
        return resp;
    }

    // 默认行为：拼接参数并返回
    if (argc > 1)
    {
        resp[0] = '\0';
        for (int i = 1; i < argc; i++)
        {
            strncat(resp, argv[i], sizeof(resp) - strlen(resp) - 1);
            if (i < argc - 1)
                strncat(resp, " ", sizeof(resp) - strlen(resp) - 1);
        }
        strncat(resp, "\r\n", sizeof(resp) - strlen(resp) - 1);
        return resp;
    }

    snprintf(resp, sizeof(resp), "\r\n");
    return resp;
}

char* device_handler(int argc, char **argv)
{
    if (argc == 2 &&
       (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
                 "Usage: device\r\n"
                 "\r\n"
                 "Show device model, name and serial number.\r\n");
        return resp;
    }

    snprintf(resp, sizeof(resp),
             "Name  : DAQ63050 process-signal-source\r\n"
             "UID   : %08X%08X%08X\r\n",
             (unsigned)g_vars.uid[0],
             (unsigned)g_vars.uid[1],
             (unsigned)g_vars.uid[2]);
    return resp;
}


static bool is_integer(const char *str)
{
    if (str == NULL || *str == '\0') return false;
    if (*str == '-' || *str == '+') str++;
    if (*str == '\0') return false;
    while (*str)
    {
        if (!isdigit((unsigned char)*str)) return false;
        str++;
    }
    return true;
}

char* mode_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
            "Usage: mode [dc|square|sine]\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "Get or set the output mode.\r\n"
            "  dc     - DC output mode\r\n"
            "  square - 1kHz square wave mode\r\n"
            "  sine   - 1kHz sine wave mode\r\n");
        return resp;
    }

    if (argc == 1)
    {
        switch (g_vars.device_mode)
        {
        case MODE_DC:    snprintf(resp, sizeof(resp), "DC\r\n");    break;
        case MODE_SQUARE: snprintf(resp, sizeof(resp), "SQUARE\r\n"); break;
        case MODE_SINE:   snprintf(resp, sizeof(resp), "SINE\r\n");   break;
        default:          snprintf(resp, sizeof(resp), "UNKNOWN\r\n"); break;
        }
        return resp;
    }

    if (argc == 2)
    {
        device_mode_t target;
        if (strcmp(argv[1], "dc") == 0)
            target = MODE_DC;
        else if (strcmp(argv[1], "square") == 0)
            target = MODE_SQUARE;
        else if (strcmp(argv[1], "sine") == 0)
            target = MODE_SINE;
        else
        {
            snprintf(resp, sizeof(resp),
                "ERR: unknown mode '%s', expecting dc, square or sine\r\n", argv[1]);
            return resp;
        }

        if (switch_mode(target))
        {
            snprintf(resp, sizeof(resp), "OK\r\n");
        }
        else
        {
            snprintf(resp, sizeof(resp), "ERR: switch mode failed\r\n");
        }
        return resp;
    }

    snprintf(resp, sizeof(resp), "Invalid option. Try 'mode -h'\r\n");
    return resp;
}

char* offset_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
            "Usage: offset [value]\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "Get or set the DC offset in mV (range 0 ~ 4095).\r\n");
        return resp;
    }

    if (argc == 1)
    {
        snprintf(resp, sizeof(resp), "%u\r\n", (unsigned)g_vars.dac_offset);
        return resp;
    }

    if (argc == 2)
    {
        if (!is_integer(argv[1]))
        {
            snprintf(resp, sizeof(resp),
                "ERR: invalid number '%s'\r\n", argv[1]);
            return resp;
        }

        int value = atoi(argv[1]);
        if (value < 0 || value > 4095)
        {
            snprintf(resp, sizeof(resp),
                "ERR: value out of range [0, 4095]\r\n");
            return resp;
        }

        if (set_dcoffset((int16_t)value))
        {
            snprintf(resp, sizeof(resp), "OK\r\n");
        }
        else
        {
            int32_t peak   = (int32_t)value + (int32_t)g_vars.dac_vpp / 2;
            int32_t valley = (int32_t)value - (int32_t)g_vars.dac_vpp / 2;
            snprintf(resp, sizeof(resp),
                "ERR: peak(%ld) or valley(%ld) exceeds range [0, 4095]\r\n",
                (long)peak, (long)valley);
        }
        return resp;
    }

    snprintf(resp, sizeof(resp), "Invalid option. Try 'offset -h'\r\n");
    return resp;
}

char* vpp_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
            "Usage: vpp [value]\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "Get or set the peak-to-peak voltage in mV (range 0 ~ 4095).\r\n");
        return resp;
    }

    if (argc == 1)
    {
        snprintf(resp, sizeof(resp), "%u\r\n", (unsigned)g_vars.dac_vpp);
        return resp;
    }

    if (argc == 2)
    {
        if (!is_integer(argv[1]))
        {
            snprintf(resp, sizeof(resp),
                "ERR: invalid number '%s'\r\n", argv[1]);
            return resp;
        }

        int value = atoi(argv[1]);
        if (value < 0 || value > 4095)
        {
            snprintf(resp, sizeof(resp),
                "ERR: value out of range [0, 4095]\r\n");
            return resp;
        }

        if (set_vpp((int16_t)value))
        {
            snprintf(resp, sizeof(resp), "OK\r\n");
        }
        else
        {
            int32_t peak   = (int32_t)g_vars.dac_offset + (int32_t)value / 2;
            int32_t valley = (int32_t)g_vars.dac_offset - (int32_t)value / 2;
            snprintf(resp, sizeof(resp),
                "ERR: peak(%ld) or valley(%ld) exceeds range [0, 4095]\r\n",
                (long)peak, (long)valley);
        }
        return resp;
    }

    snprintf(resp, sizeof(resp), "Invalid option. Try 'vpp -h'\r\n");
    return resp;
}

char* status_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
            "Usage: status\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "Show current device status.\r\n");
        return resp;
    }

    const char *mode_str;
    switch (g_vars.device_mode)
    {
    case MODE_DC:     mode_str = "DC";     break;
    case MODE_SQUARE: mode_str = "SQUARE"; break;
    case MODE_SINE:   mode_str = "SINE";   break;
    default:          mode_str = "UNKNOWN"; break;
    }

    snprintf(resp, sizeof(resp),
        "Tick  : %lu ms\r\n"
        "Mode  : %s\r\n"
        "Offset: %u mV\r\n"
        "Vpp   : %u mV\r\n",
        (unsigned long)g_vars.tick,
        mode_str,
        (unsigned)g_vars.dac_offset,
        (unsigned)g_vars.dac_vpp);
    return resp;
}
