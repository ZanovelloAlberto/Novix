#include "fat.h"
#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "ctype.h"

#define MEMORY_FAT_ADDR     ((void*)0x20000)
#define MEMORY_ROOTDIR_ADDR     ((void*)0x23000)
#define MEMORY_FATBUFFER_ADDR     ((void*)0x26000)

#define MAX_PATH_SIZE           256

typedef struct{
    bool isDirectory;
    uint32_t size;
}FAT_Info;

FAT_BootSector g_bootSector;
uint32_t g_DataSectionLba;

bool FAT_ReadBootSector(DISK* disk)
{
    return DISK_ReadSectors(disk, 0, 1, &g_bootSector);
}

bool FAT_ReadFat(DISK* disk)
{
    return DISK_ReadSectors(disk, g_bootSector.ReservedSectors, g_bootSector.SectorsPerFat, MEMORY_FAT_ADDR);
}

bool FAT_ReadRootDir(DISK* disk)
{
    uint32_t rootDirLba = g_bootSector.ReservedSectors + g_bootSector.SectorsPerFat * g_bootSector.FatCount;
    uint32_t rootDirSize = (sizeof(FAT_DirectoryEntry) * g_bootSector.DirEntryCount) / g_bootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSize;

    return DISK_ReadSectors(disk, rootDirLba, rootDirSize, MEMORY_ROOTDIR_ADDR);
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_bootSector.SectorsPerCluster;
}

uint32_t FAT_NextCluster(uint32_t currentCluster)
{    
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(MEMORY_FAT_ADDR + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(MEMORY_FAT_ADDR + fatIndex)) >> 4;
}

bool FAT_Initialize(DISK* disk)
{
    if (!FAT_ReadBootSector(disk))
    {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    if (!FAT_ReadFat(disk))
    {
        printf("FAT: read FAT failed\r\n");
        return false;
    }

    if (!FAT_ReadRootDir(disk))
    {
        printf("FAT: read Root Dir failed\r\n");
        return false;
    }

    return true;
}

uint32_t FAT_CheckExistance(void* buffer, const char* fatName, FAT_Info *infoFile)
{
    FAT_DirectoryEntry *entry = buffer;

    for(int i = 0; i < g_bootSector.DirEntryCount; i++)
    {
        if (memcmp(fatName, entry->Name, 11) == 0)
        {
            infoFile->size = entry->Size;
            return entry->FirstClusterLow;
        }

        entry++;
    }

    return 0;
}

bool FAT_LoadClustersLoop(DISK *disk, uint32_t cluster)
{
    uint32_t nextCluster = cluster;
    void* buffer = MEMORY_FATBUFFER_ADDR;
    
    do{
        DISK_ReadSectors(disk, FAT_ClusterToLba(nextCluster), g_bootSector.SectorsPerCluster, buffer);
        buffer += g_bootSector.SectorsPerCluster * g_bootSector.BytesPerSector;
        nextCluster = FAT_NextCluster(nextCluster);

    }while(nextCluster < 0xFF8);
}

bool FAT_LoadFile(DISK *disk, const char* path, void* dataOut)
{
    char name[MAX_PATH_SIZE];
    void *dirBuffer = MEMORY_ROOTDIR_ADDR;
    FAT_Info infoFile;

    // ignore leading slash
    if (path[0] == '/')
        path++;

    bool isLast = false;

    while (!isLast) {
        // extract next file name from path
        const char* delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        char fatName[12];

        // convert from name to fat name
        memset(fatName, ' ', sizeof(fatName));
        fatName[11] = '\0';

        const char* ext = strchr(name, '.');
        if (ext == NULL)
            ext = name + 11;

        for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        {
            fatName[i] = toupper(name[i]);
        }

        if (ext != name + 11)
        {
            for (int i = 0; i < 3 && ext[i + 1]; i++)
                fatName[i + 8] = toupper(ext[i + 1]);
        }

        uint32_t cluster = FAT_CheckExistance(dirBuffer, fatName, &infoFile);
        if(cluster <= 1){
            printf("the file doesn't exist");
            return false;
        }
        else
            FAT_LoadClustersLoop(disk, cluster);

        dirBuffer = MEMORY_FATBUFFER_ADDR;
    }

    memcpy(dataOut, MEMORY_FATBUFFER_ADDR, infoFile.size);

    return true;
}
