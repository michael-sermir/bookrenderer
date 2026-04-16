#error I have not implemented anything useful yet.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <dirent.h>

char* title;
char* description;
char* author;

size_t chapterCount;
char** chapters;
char** chapterTexts;

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
    off_t difference = endOfLine - fileContents;
    author = strndup(fileContents, difference);
    if(author == NULL)
    {
        perror("Failed to copy author string");
        exit(EXIT_FAILURE);
    }

    description = strndup(endOfLine + 1, stats.st_size - difference - 1);
    if(description == NULL)
    {
        perror("Failed to copy description string");
        exit(EXIT_FAILURE);
    }
}

void processChapters(const char* const path, size_t pathLength)
{
    char chaptersPath[pathLength + 10];
    memcpy(chaptersPath, path, pathLength);
    memcpy(chaptersPath + pathLength, "/chapters", 10);

    DIR* chapters = opendir(chaptersPath);
    if(chapters == NULL)
    {
        perror("Failed to open the chapters directory");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    while((entry = readdir(chapters)) != NULL)
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // TODO: Implement logic here to detect the chapter number and store it
        // in the chapter names and contents arrays by that index, that way we
        // can easily scan for holes and more easily link chapters once we
        // implement that.
    }
}

int processBook(const char* const path)
{
    title = (char*)strrchr(path, '/');
    if(title == NULL)
    {
        title = (char*)path;
    }

    bool gotMetadata = false;
    bool gotChapters = false;

    DIR* inputDirectory = opendir(path);
    if(inputDirectory == NULL)
    {
        perror("Failed to create output directory");
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

    if(mkdir(title, 0777) != 0)
    {
        perror("Failed to create output directory");
        return 1;
    }

    DIR* outputDirectory = opendir(title);
    if(outputDirectory == NULL)
    {
        perror("Failed to open output directory");
        return 1;
    }

    closedir(outputDirectory);
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

