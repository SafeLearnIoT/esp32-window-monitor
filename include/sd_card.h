/* Connect the SD card to the following pins:
 * SD Card | ESP32
 *    MISO    MISO
 *    SCK     SCK
 *    ss      D6
 *    MOSI    MOSI
 *    GND     GND
 *    +5V     VCC
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "communication.h"

void list_dir(fs::FS &fs, const char *dirname, uint8_t levels);

void create_dir(fs::FS &fs, const char *path);

void remove_dir(fs::FS &fs, const char *path);

String read_file(fs::FS &fs, const char *path);

void write_file(fs::FS &fs, const char *path, const char *message);

void append_file(fs::FS &fs, const char *path, const char *message);

void rename_file(fs::FS &fs, const char *path1, const char *path2);

void delete_file(fs::FS &fs, const char *path);

bool file_exists(fs::FS &fs, const char *path);

void checkAndCleanFileSystem(fs::FS &fs);