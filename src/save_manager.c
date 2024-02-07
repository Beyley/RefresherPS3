#include <SDL2/SDL.h>
#include <unistd.h>
#include <cJSON.h>
#include <sys/stat.h>

#include "assert.h"
#include "server_list.h"

#define GAME_DIR "/dev_hdd0/game/REFRESHER/"
#define SAVE_FILE_PATH GAME_DIR "refresher_servers.json"

#define JSON_NAME_KEY "name"
#define JSON_URL_KEY "url"
#define JSON_PATCH_DIGEST_KEY "patch_digest"

server_list_entry *load_saved_servers()
{
    // If the game dir does not exist, then create it
    if (access(GAME_DIR, F_OK) != 0)
    {
        SDL_Log("Game dir does not exist, creating it");

        ASSERT_ZERO(mkdir(GAME_DIR, 0777), "Unable to create game dir");
    }

    // If the save file does not exist, then create it
    if (access(SAVE_FILE_PATH, F_OK) != 0)
    {
        SDL_Log("Save file does not exist, creating it");

        FILE *save_file = fopen(SAVE_FILE_PATH, "w");
        ASSERT_NONZERO(save_file, "Unable to create save file");

        fputs("[]", save_file);

        fclose(save_file);

        // Early return to save the hassle of reading the file
        return NULL;
    }

    // Try to open the save file
    FILE *save_file = fopen(SAVE_FILE_PATH, "r");
    if (save_file == NULL)
    {
        SDL_Log("Unable to open save file for reading");
        return NULL;
    }

    // Get its length
    fseek(save_file, 0, SEEK_END);
    size_t save_file_size = ftell(save_file);
    fseek(save_file, 0, SEEK_SET);

    // Allocate memory to store the read file
    char *save_file_data = (char *)malloc(save_file_size);
    ASSERT_NONZERO(save_file_data, "Unable to allocate memory for save file");

    // Read the file
    size_t bytes_to_write = save_file_size;
    while (bytes_to_write > 0)
    {
        const int written = save_file_size - bytes_to_write;

        bytes_to_write -= fread(save_file_data + written, sizeof(char), save_file_size - written, save_file);
    }

    // Close the file
    fclose(save_file);

    // Parse the JSON
    cJSON *json = cJSON_Parse(save_file_data);
    // If parsing the JSON failed, return NULL, as this should not be a failure condition
    if (json == NULL)
    {
        SDL_Log("Error parsing JSON: %s", cJSON_GetErrorPtr());

        return NULL;
    }

    // If the JSON is not an array, return NULL, as this should not be a failure condition
    if (!cJSON_IsArray(json))
    {
        SDL_Log("JSON is not an array");

        cJSON_Delete(json);

        return NULL;
    }

    server_list_entry *first_entry = NULL;
    server_list_entry *current_entry = NULL;

    // Iterate over the array
    cJSON *server = NULL;
    cJSON_ArrayForEach(server, json)
    {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(server, JSON_NAME_KEY);
        cJSON *url = cJSON_GetObjectItemCaseSensitive(server, JSON_URL_KEY);
        cJSON *patch_digest = cJSON_GetObjectItemCaseSensitive(server, JSON_PATCH_DIGEST_KEY);

        // If any of the required fields are missing or the wrong type, skip this entry
        if (!cJSON_IsString(name) || !cJSON_IsString(url) || !cJSON_IsNumber(patch_digest))
        {
            SDL_Log("Invalid server entry");
            continue;
        }

        SDL_Log("Name: %s, URL: %s, Patch Digest: %d", name->valuestring, url->valuestring, patch_digest->valueint);

        // Create a new entry with the values from the file
        server_list_entry *entry = server_list_entry_create(strdup(name->valuestring), strdup(url->valuestring), patch_digest->valueint);

        // If the first entry is not set, set it
        if (first_entry == NULL)
            first_entry = entry;
        // Otherwise, set the next entry of the current entry to the new entry
        else
            current_entry->next = entry;

        // Update the current entry
        current_entry = entry;
    }

    cJSON_Delete(json);

    return first_entry;
}

int save_servers(server_list_entry *first_entry)
{
    // Create a JSON array
    cJSON *json = cJSON_CreateArray();
    ASSERT_NONZERO(json, "Unable to create JSON array");

    // Iterate over the linked list
    server_list_entry *current_entry = first_entry;
    while (current_entry != NULL)
    {
        // Create a JSON object
        cJSON *server = cJSON_CreateObject();

        // Add the name, URL and patch digest to the object
        cJSON_AddItemToObject(server, JSON_NAME_KEY, cJSON_CreateString(current_entry->name));
        cJSON_AddItemToObject(server, JSON_URL_KEY, cJSON_CreateString(current_entry->url));
        cJSON_AddItemToObject(server, JSON_PATCH_DIGEST_KEY, cJSON_CreateNumber(current_entry->patch_digest));

        // Add the object to the array
        cJSON_AddItemToArray(json, server);

        // Move to the next entry
        current_entry = current_entry->next;
    }

    // Convert the JSON to a string
    char *json_string = cJSON_Print(json);
    ASSERT_NONZERO(json_string, "Unable to convert JSON to string");

    // Write the string to the save file
    FILE *save_file = fopen(SAVE_FILE_PATH, "w");
    if (save_file == NULL)
    {
        SDL_Log("Unable to open save file for writing");
        return -1;
    }

    // Write the string to the file
    fputs(json_string, save_file);

    // Close the file
    fclose(save_file);

    // Free the json object
    cJSON_Delete(json);

    // Free the string
    cJSON_free(json_string);

    return 0;
}