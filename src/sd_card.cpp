#include "sd_card.h"

void list_dir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                list_dir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void create_dir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
}

void remove_dir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

String read_file(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }

    String output = "";
    while (file.available())
    {
        output += file.readStringUntil('\n');
        output += '\n';
    }
    file.close();
    return output;
}

void write_file(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
}

void append_file(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
}

void rename_file(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("File renamed");
    }
    else
    {
        Serial.println("Rename failed");
    }
}

void delete_file(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

bool file_exists(fs::FS &fs, const char *path)
{
    Serial.printf("Checking file: %s\r\n", path);
    if (fs.exists(path))
    {
        Serial.println("- file exists");
        return true;
    }
    else
    {
        Serial.println("- file doesn't exists");
        return false;
    }
}

void checkAndCleanFileSystem(fs::FS &fs)
{
    unsigned long totalBytes = SD.totalBytes();
    unsigned long usedBytes = SD.usedBytes();

    float freeSpacePercentage = ((float)(totalBytes - usedBytes) / totalBytes) * 100;

    Serial.print("Free space percentage: ");
    Serial.println(freeSpacePercentage);

    if (freeSpacePercentage < 10)
    {
        File root = fs.open("/");
        if (!root)
        {
            Serial.println("- failed to open directory");
            return;
        }
        if (!root.isDirectory())
        {
            Serial.println(" - not a directory");
            return;
        }

        File file = root.openNextFile();
        while (file)
        {
            if (!file.isDirectory())
            {
                if (!String(file.name()).isEmpty() && Communication::get_instance()->is_older_than_five_days(file.name()))
                {
                    fs.remove(file.name());
                }
            }
            file = root.openNextFile();
        }
    }
    else
    {
        Serial.println("Sufficient free space available.");
    }
}