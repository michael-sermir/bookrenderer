#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

char* site;
size_t siteLength;

char* root;
size_t rootLength;

size_t linkCount;
struct {
    size_t titleLength;
    char* title;
    size_t pageLength;
    char* page;
}* links;

char* title;
size_t titleLength;

char* description;
size_t descriptionLength;

char* author;
size_t authorLength;

char* license;
size_t licenseLength;

size_t chapterCount;
struct {
    size_t length;
    char* title;
}* chapterTitles;
struct {
    size_t length;
    char* body;
}* chapterTexts;

void printUsage(const char* const argv0)
{
    (void)printf("Usage: %s [OPTIONS] <sitemap> <book>\n", argv0);
    (void)puts(
        "Options:\n"
        "    --help: Display this help message and exit."
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

    struct stat stats;
    if(fstat(fileno(map), &stats) != 0)
    {
        perror("Failed to stat map file");
        exit(EXIT_FAILURE);
    }
    
    char fileContents[stats.st_size + 1];
    fread(fileContents, 1, stats.st_size, map);
    fileContents[stats.st_size] = 0;

    char* endOfLine = (char*)strchr(fileContents, '\n');
    siteLength = endOfLine - fileContents;
    site = strndup(fileContents, siteLength);

    char* endOfLine2 = (char*)strchr(endOfLine + 1, '\n');
    rootLength = endOfLine2 - endOfLine - 1;
    root = strndup(endOfLine + 1, rootLength);

    for(char* endOfNextLine = (char*)strchr(endOfLine2 + 1, '\n'),
        *endOfLastLine = endOfLine2 + 1; endOfNextLine != NULL;)
    {
        linkCount++;
        if(linkCount == 1)
            links = malloc(sizeof(*links));
        else
            links = realloc(links, sizeof(*links) * linkCount);

        char* endOfTitle = strchr(endOfLastLine, ' ');
        links[linkCount - 1].titleLength = endOfTitle - endOfLastLine;
        links[linkCount - 1].title = strndup(endOfLastLine, links[linkCount - 1].titleLength);

        endOfLastLine = endOfNextLine + 1;
        endOfNextLine = (char*)strchr(endOfLastLine, '\n');
        links[linkCount - 1].pageLength = endOfLastLine - endOfTitle - 2;
        links[linkCount - 1].page = strndup(endOfTitle + 1, links[linkCount - 1].pageLength);
    }

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

    chapterTitles = malloc(sizeof(*chapterTitles) * 99);
    if(chapterTitles == NULL)
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
        chapterTitles[chapterIndex].length = strlen(entry->d_name);
        chapterTitles[chapterIndex].title = strndup(entry->d_name,
            chapterTitles[chapterIndex].length);

        char chapterPath[pathLength + chapterTitles[chapterIndex].length + 11];
        memcpy(chapterPath, chaptersPath, pathLength + 9);
        chapterPath[pathLength + 9] = '/';
        memcpy(chapterPath + pathLength + 10, entry->d_name, chapterTitles[chapterIndex].length);
        chapterPath[pathLength + chapterTitles[chapterIndex].length + 10] = 0;

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
    }
}

void outputHeader(FILE* file)
{
    fwrite("<!DOCTYPE html><html><head><title>", 1, 34, file);
    fwrite(site, 1, siteLength, file);
    fwrite("</title><meta name=\"color-scheme\" content=\"dark\"></head><body style=\"font-family:monospace;padding:1%\"><h1 style=\"padding:0;margin:0\">", 1, 134, file);
    fwrite(site, 1, siteLength, file);
    fwrite("</h1><span>", 1, 11, file);

    for(size_t i = 0; i < linkCount; ++i)
    {
        fwrite("<a href=\"", 1, 9, file);
        fwrite(links[i].page, 1, links[i].pageLength, file);
        fwrite("\" style=\"color:initial\">", 1, 24, file);
        fwrite(links[i].title, 1, links[i].titleLength, file);
        fwrite("</a> ", 1, 5, file);
    }

    fwrite("</span>", 1, 7, file);
}

void outputBook(char* outputDirectoryPath)
{
    if(chdir(outputDirectoryPath) != 0)
    {
        perror("Failed to cd into output directory");
        exit(EXIT_FAILURE);
    }

    FILE* indexFile = fopen("index.html", "wb");
    if(indexFile == NULL)
    {
        perror("Failed to create index.html file");
        exit(EXIT_FAILURE);
    }

    outputHeader(indexFile);

    fwrite("<p style=\"margin-bottom:20px\">", 1, 30, indexFile);
    fwrite(title, 1, titleLength, indexFile);
    fwrite(" index! (<a href=\"", 1, 18, indexFile);
    fwrite(root, 1, rootLength, indexFile);
    fwrite("\" style=\"color:initial\">back</a>)</p><p>table of contents:</p><ol>", 1, 66, indexFile);

    for(size_t i = 0; i < chapterCount; ++i)
    {
        fwrite("<li><a href=\"./", 1, 15, indexFile);
        fwrite(chapterTitles[i].title, 1, chapterTitles[i].length, indexFile);
        fwrite(".html\" style=\"color:initial\">", 1, 29, indexFile);

        char chapterFilename[chapterTitles[i].length + 6];
        memcpy(chapterFilename, chapterTitles[i].title, chapterTitles[i].length);
        memcpy(chapterFilename + chapterTitles[i].length, ".html", 5);
        chapterFilename[chapterTitles[i].length + 5] = 0;

        FILE* chapterFile = fopen(chapterFilename, "wb");
        if(chapterFile == NULL)
        {
            perror("Unable to open chapter file");
            exit(EXIT_FAILURE);
        }

        outputHeader(chapterFile);
        fwrite("<p style=\"margin-bottom:20px\">", 1, 30, chapterFile);

        char* wordStart = chapterTitles[i].title + 3;
        char* wordEnd = (char*)strchr(wordStart, '_');
        do
        {
            if(wordEnd != NULL)
            {
                fwrite(wordStart, 1, wordEnd - wordStart, indexFile);
                fwrite(" ", 1, 1, indexFile);
                fwrite(wordStart, 1, wordEnd - wordStart, chapterFile);
                fwrite(" ", 1, 1, chapterFile);
            }
            else
            {
                fwrite(wordStart, 1, chapterTitles[i].length - (wordStart -
                    chapterTitles[i].title), indexFile);
                fwrite(wordStart, 1, chapterTitles[i].length - (wordStart -
                    chapterTitles[i].title), chapterFile);
                break;
            }

            wordStart = wordEnd + 1;
            wordEnd = (char*)strchr(wordStart, '_');
        }
        while(true);
        fwrite("</a></li>", 1, 9, indexFile);

        fwrite("! (<a href=\"", 1, 12, chapterFile);
        if(i == 0)
            fwrite("./index.html\" style=\"color:initial\">back</a>)", 1, 45, chapterFile);
        else
            fprintf(chapterFile, "./%s.html\" style=\"color:initial\">back</a>)", chapterTitles[i - 1].title);
        fwrite(" (<a href=\"index.html\" style=\"color:initial\">root</a>)</p><div style=\"width:80ch;overflow-wrap:break-word\">", 1, 107, chapterFile);

        char* paragraphStart = chapterTexts[i].body;
        char* paragraphEnd = strstr(paragraphStart, "\n\n");
        if(paragraphEnd == NULL)
            paragraphEnd = chapterTexts[i].body + chapterTexts[i].length - 1;

        do
        {
            char* lineStart = paragraphStart;
            char* lineEnd = strchr(paragraphStart, '\n');

            bool codegraph = false;
            bool linkgraph = false;
            if (*lineStart == '[' && *(lineStart + 1) == '<')
            {
                lineStart += 2;
                char* insetEnd = strchr(lineStart, ':');

                if(strncmp(lineStart, "code:", insetEnd - lineStart) == 0)
                {
                    fwrite("<pre><code type=\"", 1, 17, chapterFile);
                    codegraph = true;

                    char* typeStart = lineStart + 5;
                    char* typeEnd = strchr(typeStart, '>');
                    if(typeEnd == NULL)
                    {
                        fputs("Failed to find end for code type.\n", stderr);
                        exit(EXIT_FAILURE);
                    }

                    fwrite(typeStart, 1, typeEnd - typeStart, chapterFile);
                    fwrite("\">", 1, 2, chapterFile);
                }
                else if(strncmp(lineStart, "next:", insetEnd - lineStart) == 0)
                {
                    linkgraph = true;
                    fwrite("<p>", 1, 3, chapterFile);
                }
                else
                {
                    fprintf(stderr, "Unrecognized inset type '%.*s'.\n", (int)(insetEnd- lineStart), lineStart);
                    exit(EXIT_FAILURE);
                }

                paragraphStart = lineEnd + 1;
                lineStart = paragraphStart;
                lineEnd = strchr(paragraphStart, '\n');
            }
            else
                fwrite("<p>", 1, 3, chapterFile);

            do
            {
                if(lineEnd != NULL && lineEnd != paragraphEnd)
                {
                    fwrite(lineStart, 1, lineEnd - lineStart, chapterFile);
                    if(codegraph)
                        fwrite("\n", 1, 1, chapterFile);
                    else
                        fwrite(" ", 1, 1, chapterFile);
                }
                else
                {
                    if(!codegraph && !linkgraph)
                    {
                        if(paragraphEnd == chapterTexts[i].body + chapterTexts[i].length - 1)
                        {
                            fwrite(lineStart, 1, chapterTexts[i].length - (lineStart - chapterTexts[i].body), chapterFile);
                        }
                        else
                        {
                            fwrite(lineStart, 1, paragraphEnd - lineStart, chapterFile);
                        }
                        fwrite("</p>", 1, 4, chapterFile);
                    }
                    else if(codegraph)
                        fwrite("</code></pre>", 1, 13, chapterFile);
                    else if(linkgraph)
                    {
                        fwrite(" <a href=\"", 1, 10, chapterFile);
                        if(i == chapterCount - 1)
                        {
                            fputs("Dangling next link on last chapter.", stderr);
                            exit(EXIT_FAILURE);
                        }

                        fwrite(chapterTitles[i + 1].title, 1, chapterTitles[i + 1].length, chapterFile);
                        fwrite(".html\" style=\"color:initial\">next</a>.</p>", 1, 42, chapterFile);
                    }

                    break;
                }

                lineStart = lineEnd + 1;
                lineEnd = strchr(lineStart, '\n');
            }
            while(true);

            if(paragraphEnd == chapterTexts[i].body + chapterTexts[i].length - 1)
                break;

            paragraphStart = paragraphEnd + 2;
            paragraphEnd = strstr(paragraphStart, "\n\n");
            if(paragraphEnd == NULL)
                paragraphEnd = chapterTexts[i].body + chapterTexts[i].length - 1;
        }
        while(true);
        fwrite("</div>", 1, 6, chapterFile);

        fclose(chapterFile);
    }

    fwrite("</ol></body></html>", 1, 19, indexFile);
    fclose(indexFile);
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
    if(argc != 3)
    {
        (void)fputs("Malformed arguments.\n", stderr);
        printUsage(argv[0]);
        return 1;
    }

    int i = 1;
    for(; i < argc - 2; ++i)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
            printUsage(argv[0]);
            return 0;
        }

        (void)fputs("Unrecognized argument provided.\n", stderr);
        return 1;
    }

    processSitemap(argv[i]);
    return processBook(argv[i + 1]);
}

