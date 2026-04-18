#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

char* title;
size_t titleLength;

char* description;
size_t descriptionLength;

char* author;
size_t authorLength;

char* license;
size_t licenseLength;

size_t chapterCount;
char** chapters;
struct {
    size_t length;
    char* body;
}* chapterTexts;

void printUsage(const char* const argv0)
{
    (void)printf("Usage: %s [OPTIONS] book\n", argv0);
    (void)puts(
        "Options:\n"
        "    --help: Display this help message and exit.\n"
        "    --site [sitemap]: Specify a sitemap file."
    );
}

void processSitemap(const char* const path)
{
    FILE* map = fopen(path, "rb");
    if(map == NULL)
    {
        perror("Failed to open map file");
        exit(EXIT_FAILURE);
    }

    // TODO: This.

    (void)fclose(map);
}

void processMetadata(const char* const path, size_t pathLength)
{
    char metadataPath[pathLength + 10];
    memcpy(metadataPath, path, pathLength);
    memcpy(metadataPath + pathLength, "/metadata", 10);
    
    FILE* metadata = fopen(metadataPath, "rb");
    if(metadata == NULL)
    {
        perror("Failed to open metadata file");
        exit(EXIT_FAILURE);
    }

    struct stat stats;
    if(fstat(fileno(metadata), &stats) != 0)
    {
        perror("Failed to stat metadata file");
        exit(EXIT_FAILURE);
    }

    char fileContents[stats.st_size + 1];
    if(fread(fileContents, 1, stats.st_size, metadata) != stats.st_size)
    {
        perror("Failed to read from metadata file");
        exit(EXIT_FAILURE);
    }
    fileContents[stats.st_size] = 0;
    fclose(metadata);

    char* endOfLine = strchr(fileContents, '\n');
    authorLength = endOfLine - fileContents;
    author = strndup(fileContents, authorLength);
    if(author == NULL)
    {
        perror("Failed to copy author string");
        exit(EXIT_FAILURE);
    }

    char* endOfLine2 = strchr(endOfLine + 1, '\n');
    descriptionLength = endOfLine2 - endOfLine - 1;
    description = strndup(endOfLine + 1, descriptionLength);
    if(description == NULL)
    {
        perror("Failed to copy description string");
        exit(EXIT_FAILURE);
    }

    licenseLength = stats.st_size - (endOfLine2 - fileContents) - 1;
    license = strndup(endOfLine2 + 1, licenseLength);
    if(license == NULL)
    {
        perror("Failed to copy license string");
        exit(EXIT_FAILURE);
    }
}

void processChapters(const char* const path, size_t pathLength)
{
    char chaptersPath[pathLength + 10];
    memcpy(chaptersPath, path, pathLength);
    memcpy(chaptersPath + pathLength, "/chapters", 10);

    DIR* chapterDirectory = opendir(chaptersPath);
    if(chapterDirectory == NULL)
    {
        perror("Failed to open the chapters directory");
        exit(EXIT_FAILURE);
    }

    chapters = malloc(sizeof(char*) * 99);
    if(chapters == NULL)
    {
        perror("Failed to allocate chapter array");
        exit(EXIT_FAILURE);
    }
    chapterTexts = malloc((sizeof(size_t) + sizeof(char*)) * 99);
    if(chapterTexts == NULL)
    {
        perror("Failed to allocate chapter text array");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    while((entry = readdir(chapterDirectory)) != NULL)
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        chapterCount++;
        
        uint8_t chapterIndex = ((*entry->d_name - 0x30) * 10) +
            (*(entry->d_name + 1) - 0x30);
        printf("%u\n", chapterIndex);
        size_t chapterNameLength = strlen(entry->d_name);
        chapters[chapterIndex] = strndup(entry->d_name + 3,
            chapterNameLength - 3);

        char chapterPath[pathLength + chapterNameLength + 11];
        memcpy(chapterPath, chaptersPath, pathLength + 9);
        chapterPath[pathLength + 9] = '/';
        memcpy(chapterPath + pathLength + 10, entry->d_name, chapterNameLength);
        chapterPath[pathLength + chapterNameLength + 10] = 0;

        FILE* chapterFile = fopen(chapterPath, "rb");
        if(chapterFile == NULL)
        {
            perror("Failed to open chapter file");
            exit(EXIT_FAILURE);
        }

        struct stat stats;
        if(fstat(fileno(chapterFile), &stats) != 0)
        {
            perror("Failed to stat chapter file");
            exit(EXIT_FAILURE);
        }

        chapterTexts[chapterIndex].length = stats.st_size;
        chapterTexts[chapterIndex].body = malloc(stats.st_size + 1);
        if(chapterTexts[chapterIndex].body == NULL)
        {
            perror("Failed to allocate chapter text");
            exit(EXIT_FAILURE);
        }

        if(fread(chapterTexts[chapterIndex].body, 1, stats.st_size, chapterFile) != stats.st_size)
        {
            perror("Failed to read from chapter file");
            exit(EXIT_FAILURE);
        }
        chapterTexts[chapterIndex].body[stats.st_size] = 0;
        fclose(chapterFile);

        printf("%s\n%s\n---------------\n", chapters[chapterIndex],
            chapterTexts[chapterIndex].body);
    }
}

void outputBook(char* outputDirectoryPath)
{
    // TODO: This.
    (void)outputDirectoryPath;
}

int processBook(const char* const path)
{
    title = (char*)strrchr(path, '/') + 1;
    if(title == NULL)
    {
        title = (char*)path;
    }
    title = strndup(title, strrchr(title, '.') - title);
    titleLength = strlen(title);

    bool gotMetadata = false;
    bool gotChapters = false;

    DIR* inputDirectory = opendir(path);
    if(inputDirectory == NULL)
    {
        perror("Failed to open input directory");
        return 1;
    }

    size_t pathLength = strlen(path);

    struct dirent* entry;
    while((entry = readdir(inputDirectory)) != NULL && (!gotMetadata || !gotChapters))
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if(strcmp(entry->d_name, "metadata") == 0)
        {
            if(entry->d_type != DT_REG)
            {
                (void)fputs("Malformed book directory (metadata file).\n", stderr);
                closedir(inputDirectory);
                return 1;
            }
            processMetadata(path, pathLength);
            gotMetadata = true;
        }
        else if(strcmp(entry->d_name, "chapters") == 0)
        {
            if(entry->d_type != DT_DIR)
            {
                (void)fputs("Malformed book directory (chapters directory).\n", stderr);
                closedir(inputDirectory);
                return 1;
            }
            processChapters(path, pathLength);
            gotChapters = true;
        }
    }
    closedir(inputDirectory);

    if(!gotMetadata)
    {
        (void)fputs("Malformed book directory (missing metadata file).\n", stderr);
        return 1;
    }

    if(!gotChapters)
    {
        (void)fputs("Malformed book directory (missing chapters directory).\n", stderr);
        return 1;
    }

    int code = mkdir(title, 0777);
    if(code != 0 && errno != EEXIST)
    {
        perror("Failed to create output directory");
        return 1;
    }

    outputBook(title);
    return 0;
}

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        (void)fputs("Didn't get a book name.\n", stderr);
        printUsage(argv[0]);
        return 1;
    }

    int i = 1;
    for(; i < argc - 1; ++i)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
            printUsage(argv[0]);
            return 0;
        }
        else if(strcmp(argv[i], "--site") == 0)
        {
            i++;
            if(i >= argc)
            {
                (void)fputs("Given a sitemap option with no filepath.\n", stderr);
                return 1;
            }

            processSitemap(argv[i]);
            continue;
        }

        (void)fputs("Unrecognized argument provided.\n", stderr);
        return 1;
    }

    return processBook(argv[i]);
}

