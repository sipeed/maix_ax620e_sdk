#include <asm/gpio.h>
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <hexdump.h>
#include <malloc.h>
#include <spi.h>

#include "ax_vo_common.h"

struct Image {
  const unsigned char *data;
  unsigned int len;
  char *expected;
};

#ifdef UBOOT_SPI2_SIPEED_LOGO
#include "bootlogo/bird.c"
#include "bootlogo/sipeed_logo.c"
#include "bootlogo/bootlogo_fix.c"
#include "bootlogo/bootlogo_pikvm.c"
#else
static const unsigned char sipeed_logo[] = {0};
#define sipeed_logo_len 0
static const unsigned char bird[] = {0};
#define bird_len 0
static const unsigned char bootlogo_fix[] = {0};
#define bootlogo_fix_len 0
static const unsigned char bootlogo_pikvm[] = {0};
#define bootlogo_pikvm_len 0
#endif

#define msleep(a) udelay(a * 1000)

extern int read_nanokvm_logo_index_from_boot(int *);
extern int read_int_from_boot(const char *, int *);
extern int read_string_from_boot(const char *, char *, int);

static int safe_strncmp(const char *a, size_t a_len, const char *b,
                        size_t b_len, int ignore_case) {
  if (!a || !b || a_len == 0 || b_len == 0)
    return -EINVAL;

  size_t i = 0;
  for (; i < a_len && i < b_len; i++) {
    char ca = a[i];
    char cb = b[i];

    if (ca == '\0' || cb == '\0')
      break;

    if (ignore_case) {
      ca = tolower(ca);
      cb = tolower(cb);
    }

    if (ca != cb)
      return 1;
  }

  if ((i == a_len && i < b_len && b[i] != '\0') ||
      (i == b_len && i < a_len && a[i] != '\0'))
    return 1;

  return 0;
}

static const struct Image IMAGES[] = {
    {sipeed_logo, sipeed_logo_len, "nanokvm"},
    {bootlogo_pikvm, bootlogo_pikvm_len, "pikvm"},
    {bootlogo_fix, bootlogo_fix_len, "fix"},
    {bird, bird_len, "example"},
};
static const int IMAGES_SIZE = sizeof(IMAGES) / sizeof(IMAGES[0]);

struct ugpiodev {
  int num;
  int act;
  int inact;
  bool init;
};

struct panel_spi_device {
  struct udevice *dev;
  struct spi_slave *spi;
  u32 num_init_seqs;
  const u8 *init_seq;
  struct ugpiodev dc;
  struct ugpiodev rst;
  struct ugpiodev bl;
};

static bool ugpiodev_init(struct ugpiodev *pgpiodev, int num, int act) {
#define UGPIODEV_INIT_BUF_SIZE 8
  int ret = -1;
  char buf[UGPIODEV_INIT_BUF_SIZE];

  if (num < 0 || act < 0) {
    pgpiodev->init = false;
    return false;
  }

  if (pgpiodev->init) {
    return true;
  }

  pgpiodev->num = num;
  pgpiodev->act = (act == 0) ? 0 : 1;
  pgpiodev->inact = (pgpiodev->act == 0) ? 1 : 0;
  pgpiodev->init = false;

  memset(buf, 0x00, sizeof(buf));
  snprintf(buf, sizeof(buf), "gpio%d", pgpiodev->num);

  ret = gpio_request(pgpiodev->num, buf);
  if (ret < 0) {
    VO_ERROR("gpio_request<%d> failed: %d", pgpiodev->num, ret);
    return false;
  }

  ret = gpio_direction_output(pgpiodev->num, pgpiodev->inact);
  if (ret < 0) {
    VO_ERROR("gpio_direction_output<%d> failed: %d", pgpiodev->num, ret);
    return false;
  }

  gpio_set_value(pgpiodev->num, pgpiodev->inact);
  pgpiodev->init = true;

  return true;
}

static bool ugpio_set_value(struct ugpiodev *pgpiodev, int value) {
  if (pgpiodev == NULL || !pgpiodev->init) {
    VO_ERROR("ugpio_set_value failed: NULL or deinit");
    return false;
  }

  gpio_set_value(pgpiodev->num, value);
  return true;
}

static bool ugpio_act(struct ugpiodev *pgpiodev) {
  return ugpio_set_value(pgpiodev, pgpiodev->act);
}

static bool ugpio_inact(struct ugpiodev *pgpiodev) {
  return ugpio_set_value(pgpiodev, pgpiodev->inact);
}

static bool ugpiodev_in_init(struct ugpiodev *pgpiodev, int num, int act) {
#define UGPIODEV_INIT_BUF_SIZE 8
  int ret = -1;
  char buf[UGPIODEV_INIT_BUF_SIZE];

  if (num < 0 || act < 0) {
    pgpiodev->init = false;
    return false;
  }

  if (pgpiodev->init) {
    return true;
  }

  pgpiodev->num = num;
  pgpiodev->act = (act == 0) ? 0 : 1;
  pgpiodev->inact = (pgpiodev->act == 0) ? 1 : 0;
  pgpiodev->init = false;

  memset(buf, 0x00, sizeof(buf));
  snprintf(buf, sizeof(buf), "gpio%d", pgpiodev->num);

  ret = gpio_request(pgpiodev->num, buf);
  if (ret < 0) {
    VO_ERROR("gpio_request<%d> failed: %d", pgpiodev->num, ret);
    return false;
  }

  ret = gpio_direction_input(pgpiodev->num);
  if (ret < 0) {
    VO_ERROR("gpio_direction_input<%d> failed: %d", pgpiodev->num, ret);
    return false;
  }

  pgpiodev->init = true;

  return true;
}

static bool ugpio_read(struct ugpiodev *pgpiodev) {
  int value = gpio_get_value(pgpiodev->num);
  return (value == pgpiodev->act);
}

static bool lcd_command(struct panel_spi_device *panel) {
  return ugpio_set_value(&panel->dc, 0);
}

static bool lcd_data(struct panel_spi_device *panel) {
  return ugpio_set_value(&panel->dc, 1);
}

static bool lcd_reset(struct panel_spi_device *panel, int ms) {
  if (!ugpio_inact(&panel->rst)) {
    return false;
  }
  msleep(1);

  if (!ugpio_act(&panel->rst)) {
    return false;
  }
  msleep(1);

  if (!ugpio_inact(&panel->rst)) {
    return false;
  }
  msleep(ms);

  return true;
}

static bool lcd_bl_on(struct panel_spi_device *panel) {
  return ugpio_act(&panel->bl);
}

static bool lcd_bl_off(struct panel_spi_device *panel) {
  return ugpio_inact(&panel->bl);
}

int panel_spi_init(void) {
  int i, k, len, addr, cmd_len, delay, cmd, ret, pdata_len;
  const u8 *data;
  const u8 *pdata = NULL;
  struct udevice *dev = NULL;
  struct spi_slave *slave;
  struct panel_spi_device *panel_spi;
  int image_index = -1;

  ret = uclass_first_device_err(UCLASS_PANEL_SPI, &dev);
  if (ret) {
    VO_ERROR("failed to find spi panel, ret = %d\n", ret);
    return -1;
  }

  slave = dev_get_parent_priv(dev);
  panel_spi = dev_get_uclass_priv(dev);
  len = panel_spi->num_init_seqs;
  data = panel_spi->init_seq;

  lcd_bl_off(panel_spi);
  for (k = 0; k < 1; ++k) {
    if (k != 0) {
      msleep(100);
    }
    cmd_len = 0;
    if (!lcd_reset(panel_spi, 130)) {
      VO_ERROR("lcd reset failed!");
      goto release;
    }
    for (i = 0; i < len; i += (cmd_len + 3)) {
      addr = data[i];
      delay = data[i + 1];
      cmd_len = data[i + 2];
      cmd = data[i + 3];
      if (addr != 0xFF) {
        VO_ERROR("seq parser failed in pos<%d>!\n", i);
        spi_release_bus(slave);
        return -EINVAL;
      }

      if (cmd_len + i + 3 > len) {
        print_hex_dump("cmd overflow, cmd: ", DUMP_PREFIX_NONE, 16, 1, &data[i],
                       len - i, false);
        spi_release_bus(slave);
        return -EINVAL;
      }

      pdata_len = cmd_len - 1;
      pdata = (pdata_len > 0) ? (&data[i + 4]) : (NULL);

      lcd_command(panel_spi);
      spi_xfer(slave, 8, &cmd, NULL, SPI_XFER_ONCE);
      lcd_data(panel_spi);

      if (pdata_len > 0) {
        spi_xfer(slave, pdata_len * 8, pdata, NULL, SPI_XFER_ONCE);
      }

      if (delay != 0) {
        msleep(delay);
      }
    }
  }

  image_index = 0;
  char server_buf[32];
  if (read_string_from_boot(".server.txt", server_buf, sizeof(server_buf)) >=
      0) {
    for (i = 0; i < IMAGES_SIZE; ++i) {
      if (safe_strncmp(server_buf, sizeof(server_buf), IMAGES[i].expected,
                       strlen(IMAGES[i].expected) + 1, 1) == 0) {
        image_index = i;
        break;
      }
    }
  }

  struct ugpiodev boot_btn;
  if (!ugpiodev_in_init(&boot_btn, 98, 0)) {
    printf("[E] boot_btn init failed!\n");
  } else {
    msleep(100);
    if (ugpio_read(&boot_btn)) {
      image_index = 2;
    }
  }

  lcd_command(panel_spi);
  spi_xfer(slave, 8, "\x2C", NULL, SPI_XFER_ONCE);
  lcd_data(panel_spi);
  spi_xfer(slave, IMAGES[image_index].len * 8, IMAGES[image_index].data, NULL,
           SPI_XFER_ONCE);
  lcd_bl_on(panel_spi);

release:
  spi_release_bus(slave);
  return 0;
}

static int panel_spi_dts_parse(struct panel_spi_device *panel) {
#define VALUE_SIZE 3
  int len;
  u32 vals[VALUE_SIZE];
  int gpio_num = 0;

  len = dev_read_size(panel->dev, "panel-init-seq");
  if (len > 0) {
    panel->num_init_seqs = len;
    panel->init_seq = dev_read_u8_array_ptr(panel->dev, "panel-init-seq", len);
    VO_DEBUG("spi panel-init-seq len: %d\n", len);
    print_hex_dump("spi panel-init-seq: ", DUMP_PREFIX_OFFSET, 32, 1,
                   panel->init_seq, len, false);

  } else {
    panel->num_init_seqs = 0;
    panel->init_seq = NULL;
    VO_DEBUG("spi panel-init-seq not defined\n");
  }

  len = dev_read_size(panel->dev, "dc") / sizeof(int);
  if (len != VALUE_SIZE) {
    ugpiodev_init(&panel->dc, -1, -1);
    VO_ERROR("dc not defined\n");
  } else {
    dev_read_u32_array(panel->dev, "dc", vals, len);
    gpio_num = vals[0] * 32 + vals[1];
    if (ugpiodev_init(&panel->dc, gpio_num, vals[2])) {
      printf("dc init finish\n");
    } else {
      VO_ERROR("dc init failed\n");
    }
  }

  len = dev_read_size(panel->dev, "rst") / sizeof(int);
  if (len != VALUE_SIZE) {
    ugpiodev_init(&panel->rst, -1, -1);
    VO_ERROR("rst not defined\n");
  } else {
    dev_read_u32_array(panel->dev, "rst", vals, len);
    gpio_num = vals[0] * 32 + vals[1];
    if (ugpiodev_init(&panel->rst, gpio_num, vals[2])) {
      printf("rst init finish\n");
    } else {
      VO_ERROR("rst init failed\n");
    }
  }

  len = dev_read_size(panel->dev, "bl") / sizeof(int);
  if (len != VALUE_SIZE) {
    ugpiodev_init(&panel->bl, -1, -1);
    VO_ERROR("bl not defined\n");
  } else {
    dev_read_u32_array(panel->dev, "bl", vals, len);
    gpio_num = vals[0] * 32 + vals[1];
    if (ugpiodev_init(&panel->bl, gpio_num, vals[2])) {
      printf("bl init finish\n");
    } else {
      VO_ERROR("bl init failed\n");
    }
  }

  VO_DEBUG("done\n");

  return 0;
}

static int panel_spi_probe(struct udevice *dev) {
  int ret;
  struct spi_slave *slave = dev_get_parent_priv(dev);
  struct panel_spi_device *panel_spi;

  panel_spi = dev_get_uclass_priv(dev);
  panel_spi->dev = dev;
  slave->mode |= SPI_LSB_FIRST;
  panel_spi->spi = slave;

  /* Claim spi bus */
  ret = spi_claim_bus(slave);
  if (ret) {
    VO_ERROR("failed to claim SPI_APB bus: %d\n", ret);
    return ret;
  }

  ret = panel_spi_dts_parse(panel_spi);
  if (ret)
    return ret;

  panel_spi_init();

  return 0;
}

static const struct udevice_id panel_spi_ids[] = {
    {.compatible = "axera,panel_spi"},
#ifdef UBOOT_SPI2_SIPEED_LOGO
    {.compatible = "sipeed,for_jd9853"},
#endif
    {}};

U_BOOT_DRIVER(panel_spi) = {
    .name = "panel_spi",
    .id = UCLASS_PANEL_SPI,
    .of_match = panel_spi_ids,
    .probe = panel_spi_probe,
    .priv_auto_alloc_size = sizeof(struct panel_spi_device),
};

UCLASS_DRIVER(panel_spi) = {
    .id = UCLASS_PANEL_SPI,
    .name = "panel_spi",
    .per_device_auto_alloc_size = sizeof(struct panel_spi_device),
};
