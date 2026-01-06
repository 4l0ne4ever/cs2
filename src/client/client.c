// client.c - Main Client Implementation (Phase 9)

#include "../include/ui.h"
#include "../include/network_client.h"
#include "../include/protocol.h"
#include "../include/types.h"
#include "../include/utils.h"
#include "../include/trading_challenges.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static int g_user_id = -1;
static char g_session_token[37] = {0};

// Helper function to get definition_id from instance_id
static int get_definition_id_from_instance(int instance_id, int *out_definition_id)
{
    if (!out_definition_id || instance_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_DEFINITION_ID;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", instance_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_DEFINITION_ID_DATA)
    {
        memcpy(out_definition_id, response.payload, sizeof(int));
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        return -1;
    }

    return -1;
}

// Helper function to get price trend for a skin definition
static int get_price_trend(int definition_id, PriceTrend *out_trend)
{
    if (!out_trend || definition_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_PRICE_TREND;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", definition_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_PRICE_TREND_DATA)
    {
        memcpy(out_trend, response.payload, sizeof(PriceTrend));
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        return -1;
    }

    return -1;
}

// Helper function to get price history for a skin definition
static int get_price_history(int definition_id, PriceHistoryEntry *out_history, int *count)
{
    if (!out_history || !count || definition_id <= 0)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_PRICE_HISTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", definition_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_PRICE_HISTORY_DATA)
    {
        int history_count = response.header.msg_length / sizeof(PriceHistoryEntry);
        if (history_count > 100)
            history_count = 100;

        memcpy(out_history, response.payload, sizeof(PriceHistoryEntry) * history_count);
        *count = history_count;
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        *count = 0;
        return -1;
    }

    *count = 0;
    return -1;
}

// Helper function to load full skin details from server
static int load_skin_details(int instance_id, Skin *out_skin)
{
    if (!out_skin)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_SKIN_DETAILS;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", instance_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_SKIN_DETAILS_DATA)
    {
        if (response.header.msg_length >= sizeof(Skin))
        {
            memcpy(out_skin, response.payload, sizeof(Skin));
            return 0;
        }
    }

    return -1;
}

// Helper function to get user balance and calculate total inventory value
static float get_user_balance()
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_USER_PROFILE;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_USER_PROFILE_DATA)
        {
            User user;
            memcpy(&user, response.payload, sizeof(User));
            return user.balance;
        }
    }
    return 0.0f;
}

// Helper function to calculate total inventory value
static float calculate_inventory_value()
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_INVENTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return 0.0f;

    if (receive_message_from_server(&response) != 0)
        return 0.0f;

    if (response.header.msg_type == MSG_INVENTORY_DATA)
    {
        Inventory inv;
        memcpy(&inv, response.payload, sizeof(Inventory));

        float total_value = 0.0f;
        for (int i = 0; i < inv.count && i < MAX_INVENTORY_SIZE; i++)
        {
            Skin skin;
            if (load_skin_details(inv.skin_ids[i], &skin) == 0)
            {
                total_value += skin.current_price;
            }
        }
        return total_value;
    }
    return 0.0f;
}

// Helper function to display balance info at top of screen
static void display_balance_info()
{
    float balance = get_user_balance();
    float inv_value = calculate_inventory_value();
    float total = balance + inv_value;

    move_cursor(1, 1);
    printf("Balance: %s$%.2f%s | Inventory Value: %s$%.2f%s | Total: %s$%.2f%s\n",
           COLOR_GREEN, balance, COLOR_RESET,
           COLOR_YELLOW, inv_value, COLOR_RESET,
           COLOR_BRIGHT_GREEN, total, COLOR_RESET);
    fflush(stdout);
}

// Helper function to search user by username
static int search_user_by_username(const char *username, User *out_user)
{
    if (!username || !out_user)
        return -1;

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_SEARCH_USER_BY_USERNAME;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s", username);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
        return -1;

    if (receive_message_from_server(&response) != 0)
        return -1;

    if (response.header.msg_type == MSG_SEARCH_USER_RESPONSE)
    {
        if (response.header.msg_length >= sizeof(User))
        {
            memcpy(out_user, response.payload, sizeof(User));
            return 0;
        }
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        return (int)error_code;
    }

    return -1;
}

// Forward declaration
static void send_trade_offer_ui(int to_user_id, const char *to_username);

// Authentication functions
int handle_login(const char *username, const char *password)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_LOGIN_REQUEST;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s:%s", username, password);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0)
    {
        print_error("Failed to send login request");
        return -1;
    }

    if (receive_message_from_server(&response) != 0)
    {
        print_error("Failed to receive login response");
        return -1;
    }

    if (response.header.msg_type == MSG_LOGIN_RESPONSE)
    {
        // Parse: "session_token:user_id"
        char payload_copy[MAX_PAYLOAD_SIZE + 1];
        size_t payload_len = response.header.msg_length;
        if (payload_len >= sizeof(payload_copy))
            payload_len = sizeof(payload_copy) - 1;
        memcpy(payload_copy, response.payload, payload_len);
        payload_copy[payload_len] = '\0';
        
        char *colon = strchr(payload_copy, ':');
        if (colon)
        {
            *colon = '\0';
            strncpy(g_session_token, payload_copy, sizeof(g_session_token) - 1);
            g_session_token[sizeof(g_session_token) - 1] = '\0';
            g_user_id = atoi(colon + 1);
        }
        else
        {
            // Fallback: old format (just session token)
            strncpy(g_session_token, (char *)response.payload, sizeof(g_session_token) - 1);
            g_session_token[sizeof(g_session_token) - 1] = '\0';
        }
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        if (error_code == ERR_INVALID_CREDENTIALS)
            print_error("Invalid username or password");
        else
            print_error("Login failed");
        return -1;
    }

    return -1;
}

int handle_register(const char *username, const char *password)
{
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_REGISTER_REQUEST;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s:%s", username, password);
    request.header.msg_length = strlen(request.payload);

    int send_result = send_message_to_server(&request);

    if (send_result != 0)
    {
        print_error("Failed to send register request");
        return -1;
    }

    int recv_result = receive_message_from_server(&response);

    if (recv_result != 0)
    {
        print_error("Failed to receive register response");
        return -1;
    }

    if (response.header.msg_type == MSG_REGISTER_RESPONSE)
    {
        memcpy(&g_user_id, response.payload, sizeof(uint32_t));
        print_success("Registration successful!");
        return 0;
    }
    else if (response.header.msg_type == MSG_ERROR)
    {
        uint32_t error_code;
        memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
        if (error_code == ERR_USER_EXISTS)
            print_error("Username already exists");
        else
            print_error("Registration failed");
        return -1;
    }

    return -1;
}

// Menu functions
void show_main_menu()
{
    clear_screen();
    print_header("CS2 SKIN TRADING SIMULATOR");

    printf("\n");
    print_menu_item("1. Inventory", 0, 2, 5);
    print_menu_item("2. Market", 0, 2, 6);
    print_menu_item("3. Trading", 0, 2, 7);
    print_menu_item("4. Unbox Cases", 0, 2, 8);
    print_menu_item("5. Profile", 0, 2, 9);
    print_menu_item("6. Quests & Achievements", 0, 2, 10);
    print_menu_item("7. Daily Rewards", 0, 2, 11);
    print_menu_item("8. Chat", 0, 2, 12);
    print_menu_item("9. Leaderboards", 0, 2, 13);
    print_menu_item("10. Trade Analytics", 0, 2, 14);
    print_menu_item("11. Trading Challenges", 0, 2, 15);
    print_menu_item("12. Logout", 0, 2, 16);
    print_menu_item("13. Exit", 0, 2, 17);

    printf("\n");
    print_separator(50);
    printf("Select option (1-13): ");
    fflush(stdout);
}

void show_inventory()
{
    int should_exit = 0;
    while (!should_exit)
    {
        clear_screen();
        print_header("INVENTORY");
        display_balance_info();

        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        request.header.magic = 0xABCD;
        request.header.msg_type = MSG_GET_INVENTORY;
        snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
        request.header.msg_length = strlen(request.payload);

        if (send_message_to_server(&request) != 0)
        {
            print_error("Failed to request inventory");
            wait_for_key();
            return;
        }

        if (receive_message_from_server(&response) != 0)
        {
            print_error("Failed to receive inventory");
            wait_for_key();
            return;
        }

        if (response.header.msg_type == MSG_INVENTORY_DATA)
        {
            Inventory inv;
            memcpy(&inv, response.payload, sizeof(Inventory));

            printf("\nYour Inventory (%d items):\n\n", inv.count);

            if (inv.count == 0)
            {
                print_info("Your inventory is empty");
                printf("\nPress Enter to return...");
                getchar();
                return;
            }
            else
            {
                printf("\n");
                Skin skins[MAX_INVENTORY_SIZE];
                int instance_ids[MAX_INVENTORY_SIZE];
                int valid_count = 0;

                for (int i = 0; i < inv.count && i < MAX_INVENTORY_SIZE; i++)
                {
                    int instance_id = inv.skin_ids[i];
                    if (load_skin_details(instance_id, &skins[valid_count]) == 0)
                    {
                        instance_ids[valid_count] = instance_id; // Store mapping
                        const char *rarity_color = get_rarity_color(skins[valid_count].rarity);
                        const char *stattrak = skins[valid_count].is_stattrak ? "StatTrak™ " : "";
                        const char *wear = wear_to_string(skins[valid_count].wear);
                        const char *tradable = skins[valid_count].is_tradable ? "" : " [Trade Locked]";

                        printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d) - $%.2f%s\n",
                               valid_count + 1,
                               rarity_color, rarity_to_string(skins[valid_count].rarity), COLOR_RESET,
                               skins[valid_count].is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
                               skins[valid_count].name, wear, skins[valid_count].pattern_seed, skins[valid_count].current_price,
                               tradable);
                        valid_count++;
                    }
                }

                printf("\n");
                print_separator(50);
                printf("Options:\n");
                printf("  Enter item number to sell to market\n");
                printf("  0. Back to main menu\n");
                printf("Select option: ");
                fflush(stdout);

                char choice[32];
                if (fgets(choice, sizeof(choice), stdin) == NULL)
                {
                    should_exit = 1;
                    break;
                }

                int option = atoi(choice);
                if (option == 0)
                {
                    should_exit = 1;
                    break;
                }
                else if (option >= 1 && option <= valid_count)
                {
                    // Sell item to market
                    Skin *selected_skin = &skins[option - 1];
                    if (!selected_skin->is_tradable)
                    {
                        print_error("This item is trade locked and cannot be sold");
                        sleep(2);
                        continue;
                    }

                    // Show listing fee info
                    printf("Listing fee: %s$0.50%s (refunded if sold)\n", COLOR_YELLOW, COLOR_RESET);
                    printf("Enter price for %s: $", selected_skin->name);
                    fflush(stdout);
                    char price_str[32];
                    if (fgets(price_str, sizeof(price_str), stdin) == NULL)
                        continue;

                    float price = atof(price_str);
                    if (price <= 0)
                    {
                        print_error("Invalid price");
                        sleep(2);
                        continue;
                    }

                    // Send sell request
                    memset(&request, 0, sizeof(Message));
                    memset(&response, 0, sizeof(Message));
                    request.header.magic = 0xABCD;
                    request.header.msg_type = MSG_SELL_TO_MARKET;
                    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d:%.2f", g_user_id, instance_ids[option - 1], price);
                    request.header.msg_length = strlen(request.payload);

                    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                    {
                        if (response.header.msg_type == MSG_SELL_TO_MARKET)
                        {
                            print_success("Item listed on market successfully!");
                            printf("Listing fee: %s$0.50%s deducted (will be refunded if sold)\n", COLOR_YELLOW, COLOR_RESET);
                        }
                        else if (response.header.msg_type == MSG_ERROR)
                        {
                            uint32_t error_code;
                            memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
                            if (error_code == ERR_PERMISSION_DENIED)
                                print_error("You don't own this item");
                            else if (error_code == ERR_INSUFFICIENT_FUNDS)
                                print_error("Insufficient funds for listing fee ($0.50)");
                            else if (error_code == ERR_INVALID_REQUEST)
                                print_error("Invalid request");
                            else
                                print_error("Failed to list item on market");
                        }
                    }
                    else
                    {
                        print_error("Failed to communicate with server");
                    }

                    sleep(2);
                }
                else
                {
                    print_error("Invalid option");
                    sleep(1);
                }
            }
        }
        else
        {
            print_error("Failed to load inventory");
            wait_for_key();
            return;
        }
    }
}

void show_market()
{
    int should_exit = 0;
    char search_filter[256] = {0}; // Store current search filter

    while (!should_exit)
    {
        clear_screen();
        print_header("MARKET");
        display_balance_info();

        if (strlen(search_filter) > 0)
        {
            printf("\nSearching for: %s%s%s\n", COLOR_CYAN, search_filter, COLOR_RESET);
        }
        else
        {
            printf("\nShowing all listings...\n");
        }

        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        if (strlen(search_filter) > 0)
        {
            // Search by name
            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_SEARCH_MARKET_BY_NAME;
            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%s", search_filter);
            request.header.msg_length = strlen(request.payload);
        }
        else
        {
            // Get all listings
            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_GET_MARKET_LISTINGS;
            request.header.msg_length = 0;
        }

        if (send_message_to_server(&request) != 0)
        {
            print_error("Failed to request market listings");
            wait_for_key();
            return;
        }

        if (receive_message_from_server(&response) != 0)
        {
            print_error("Failed to receive market listings");
            wait_for_key();
            return;
        }

        if (response.header.msg_type == MSG_MARKET_DATA)
        {
            MarketListing listings[100];
            int count = response.header.msg_length / sizeof(MarketListing);
            if (count > 100)
                count = 100;

            memcpy(listings, response.payload, count * sizeof(MarketListing));

            printf("\nMarket Listings (%d items):\n\n", count);

            if (count == 0)
            {
                print_info("No items on market");
                printf("\nPress Enter to return...");
                getchar();
                return;
            }
            else
            {
                // Load skin details for each listing
                Skin skins[100];
                int valid_count = 0;

                printf("Loading item details...\n");
                for (int i = 0; i < count; i++)
                {
                    if (load_skin_details(listings[i].skin_id, &skins[valid_count]) == 0)
                    {
                        valid_count++;
                    }
                }

                printf("\n");
                for (int i = 0; i < valid_count; i++)
                {
                    const char *rarity_color = get_rarity_color(skins[i].rarity);
                    const char *stattrak = skins[i].is_stattrak ? "StatTrak™ " : "";
                    const char *wear = wear_to_string(skins[i].wear);
                    const char *owner_note = (listings[i].seller_id == g_user_id) ? COLOR_YELLOW " [Your Listing]" COLOR_RESET : "";

                    printf("%s[%2d]%s ", COLOR_CYAN, i + 1, COLOR_RESET);
                    printf("%s[%s]%s ", rarity_color, rarity_to_string(skins[i].rarity), COLOR_RESET);
                    if (skins[i].is_stattrak)
                        printf("%sStatTrak™ %s", COLOR_BRIGHT_GREEN, COLOR_RESET);
                    printf("%s", skins[i].name);
                    printf(" %s(%s, Pattern #%d)%s", COLOR_DIM, wear, skins[i].pattern_seed, COLOR_RESET);
                    printf(" - %s$%.2f%s%s\n",
                           COLOR_BRIGHT_GREEN, listings[i].price, COLOR_RESET, owner_note);
                }

                printf("\n");
                print_separator(50);
                printf("Options:\n");
                printf("  %s<number>%s - Buy item (e.g., 1, 2, 3)\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
                printf("  %sV<number>%s - View item details (e.g., V1, V2)\n", COLOR_CYAN, COLOR_RESET);
                printf("  %sR<number>%s - Remove your listing (e.g., R1)\n", COLOR_YELLOW, COLOR_RESET);
                printf("  %sS%s - Search by name\n", COLOR_CYAN, COLOR_RESET);
                printf("  %sC%s - Clear search\n", COLOR_CYAN, COLOR_RESET);
                printf("  %s0%s - Back to main menu\n", COLOR_DIM, COLOR_RESET);
                printf("\nSelect option: ");
                fflush(stdout);

                char choice[32];
                if (fgets(choice, sizeof(choice), stdin) == NULL)
                {
                    should_exit = 1;
                    break;
                }

                // Remove newline
                size_t len = strlen(choice);
                if (len > 0 && choice[len - 1] == '\n')
                    choice[len - 1] = '\0';

                if (choice[0] == '0' || choice[0] == '\0')
                {
                    should_exit = 1;
                    break;
                }
                else if (choice[0] == 'S' || choice[0] == 's')
                {
                    // Search by name
                    printf("\nEnter search term: ");
                    fflush(stdout);
                    char search_term[256];
                    if (fgets(search_term, sizeof(search_term), stdin) != NULL)
                    {
                        // Remove newline
                        size_t len = strlen(search_term);
                        if (len > 0 && search_term[len - 1] == '\n')
                            search_term[len - 1] = '\0';

                        if (strlen(search_term) > 0)
                        {
                            strncpy(search_filter, search_term, sizeof(search_filter) - 1);
                            search_filter[sizeof(search_filter) - 1] = '\0';
                        }
                    }
                    // Loop will continue and show filtered results
                }
                else if (choice[0] == 'C' || choice[0] == 'c')
                {
                    // Clear search
                    search_filter[0] = '\0';
                    // Loop will continue and show all listings
                }
                else if (choice[0] == 'V' || choice[0] == 'v')
                {
                    // View item details
                    int listing_num = atoi(choice + 1);
                    if (listing_num > 0 && listing_num <= valid_count)
                    {
                        int listing_idx = listing_num - 1;
                        clear_screen();
                        print_header("ITEM DETAILS");
                        printf("\n");
                        // Display skin details
                        const char *rarity_color = get_rarity_color(skins[listing_idx].rarity);
                        const char *stattrak = skins[listing_idx].is_stattrak ? "StatTrak™ " : "";
                        const char *wear = wear_to_string(skins[listing_idx].wear);
                        printf("Item: %s[%s]%s ", rarity_color, rarity_to_string(skins[listing_idx].rarity), COLOR_RESET);
                        if (skins[listing_idx].is_stattrak)
                            printf("%sStatTrak™ %s", COLOR_BRIGHT_GREEN, COLOR_RESET);
                        printf("%s\n", skins[listing_idx].name);
                        printf("Wear: %s\n", wear);
                        printf("Pattern: #%d\n", skins[listing_idx].pattern_seed);
                        printf("Price: %s$%.2f%s", COLOR_BRIGHT_GREEN, listings[listing_idx].price, COLOR_RESET);

                        // Get definition_id and show price trend
                        // Note: listings[listing_idx].skin_id contains instance_id
                        int definition_id;
                        if (get_definition_id_from_instance(listings[listing_idx].skin_id, &definition_id) == 0)
                        {
                            PriceTrend trend;
                            if (get_price_trend(definition_id, &trend) == 0)
                            {
                                // Display price trend
                                const char *trend_color = COLOR_BRIGHT_GREEN;
                                if (trend.price_change_percent < 0)
                                    trend_color = COLOR_BRIGHT_RED;
                                else if (trend.price_change_percent > 0)
                                    trend_color = COLOR_BRIGHT_GREEN;
                                else
                                    trend_color = COLOR_DIM;

                                printf(" %s%s %.2f%%%s (24h)", trend_color, trend.trend_symbol, trend.price_change_percent, COLOR_RESET);
                            }
                        }
                        printf("\n");

                        if (listings[listing_idx].seller_id == g_user_id)
                        {
                            printf("Status: %sYour Listing%s\n", COLOR_YELLOW, COLOR_RESET);
                        }
                        
                        // Show 24h price chart if available
                        PriceHistoryEntry price_history[100];
                        int history_count = 0;
                        if (get_price_history(definition_id, price_history, &history_count) == 0 && history_count > 0)
                        {
                            printf("\n%s24h Price Chart:%s\n", COLOR_CYAN, COLOR_RESET);
                            
                            // Find min and max prices for scaling
                            float min_price = price_history[0].price;
                            float max_price = price_history[0].price;
                            for (int i = 1; i < history_count; i++)
                            {
                                if (price_history[i].price < min_price)
                                    min_price = price_history[i].price;
                                if (price_history[i].price > max_price)
                                    max_price = price_history[i].price;
                            }
                            float range = max_price - min_price;
                            if (range < 0.01f)
                                range = 0.01f; // Prevent division by zero
                            
                            // Simple ASCII graph (similar to balance graph)
                            int graph_width = 50;
                            int graph_height = 12;
                            
                            printf("Price Range: $%.2f - $%.2f\n\n", min_price, max_price);
                            
                            // Draw graph
                            for (int row = graph_height - 1; row >= 0; row--)
                            {
                                float value = min_price + (range * row / graph_height);
                                printf("%7.0f │", value);
                                
                                // Sample data points (every nth point to fit in graph_width)
                                int sample_interval = (history_count > graph_width) ? (history_count / graph_width) : 1;
                                int samples = (history_count < graph_width) ? history_count : graph_width;
                                
                                for (int col = 0; col < samples; col++)
                                {
                                    int idx = col * sample_interval;
                                    if (idx >= history_count)
                                        idx = history_count - 1;
                                    
                                    float normalized = (price_history[idx].price - min_price) / range;
                                    int bar_height = (int)(normalized * graph_height);
                                    
                                    if (bar_height == row)
                                        printf("█");
                                    else if (bar_height > row)
                                        printf("│");
                                    else
                                        printf(" ");
                                }
                                printf("\n");
                            }
                            
                            // Draw x-axis
                            printf("        └");
                            int samples = (history_count < graph_width) ? history_count : graph_width;
                            for (int i = 0; i < samples; i++)
                                printf("─");
                            printf("\n");
                            
                            // Print time labels (show first, middle, last)
                            printf("        ");
                            if (samples >= 3)
                            {
                                struct tm *timeinfo1 = localtime(&price_history[0].timestamp);
                                struct tm *timeinfo2 = localtime(&price_history[history_count / 2].timestamp);
                                struct tm *timeinfo3 = localtime(&price_history[history_count - 1].timestamp);
                                
                                char time_str1[16], time_str2[16], time_str3[16];
                                strftime(time_str1, sizeof(time_str1), "%H:%M", timeinfo1);
                                strftime(time_str2, sizeof(time_str2), "%H:%M", timeinfo2);
                                strftime(time_str3, sizeof(time_str3), "%H:%M", timeinfo3);
                                
                                printf("%-15s", time_str1);
                                printf("%-15s", time_str2);
                                printf("%-15s", time_str3);
                            }
                            printf("\n");
                        }
                        
                        printf("\nPress Enter to continue...");
                        getchar();
                    }
                    else
                    {
                        print_error("Invalid listing number");
                        sleep(1);
                    }
                }
                else if (choice[0] == 'R' || choice[0] == 'r')
                {
                    // Remove listing
                    int listing_num = atoi(choice + 1);
                    if (listing_num > 0 && listing_num <= valid_count)
                    {
                        int listing_idx = listing_num - 1;
                        if (listings[listing_idx].seller_id == g_user_id)
                        {
                            // Remove listing
                            memset(&request, 0, sizeof(Message));
                            memset(&response, 0, sizeof(Message));
                            request.header.magic = 0xABCD;
                            request.header.msg_type = MSG_REMOVE_FROM_MARKET;
                            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, listings[listing_idx].listing_id);
                            request.header.msg_length = strlen(request.payload);

                            if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                            {
                                if (response.header.msg_type == MSG_REMOVE_FROM_MARKET)
                                {
                                    print_success("Listing removed successfully!");
                                    printf("Listing fee: %s$0.50%s refunded\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
                                }
                                else if (response.header.msg_type == MSG_ERROR)
                                {
                                    uint32_t error_code;
                                    memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
                                    if (error_code == ERR_PERMISSION_DENIED)
                                        print_error("You don't own this listing");
                                    else if (error_code == ERR_ITEM_NOT_FOUND)
                                        print_error("Listing not found");
                                    else
                                        print_error("Failed to remove listing");
                                }
                            }
                            else
                            {
                                print_error("Failed to communicate with server");
                            }

                            sleep(2);
                        }
                        else
                        {
                            print_error("You can only remove your own listings");
                            sleep(2);
                        }
                    }
                    else
                    {
                        print_error("Invalid listing number");
                        sleep(1);
                    }
                }
                else
                {
                    // Buy item
                    int listing_num = atoi(choice);
                    if (listing_num > 0 && listing_num <= valid_count)
                    {
                        int listing_idx = listing_num - 1;

                        if (listings[listing_idx].seller_id == g_user_id)
                        {
                            print_error("You cannot buy your own listing");
                            sleep(2);
                            continue;
                        }

                        // Show item summary before purchase
                        clear_screen();
                        print_header("CONFIRM PURCHASE");
                        printf("\n");
                        const char *rarity_color = get_rarity_color(skins[listing_idx].rarity);
                        printf("Item: %s[%s]%s ", rarity_color, rarity_to_string(skins[listing_idx].rarity), COLOR_RESET);
                        if (skins[listing_idx].is_stattrak)
                            printf("%sStatTrak™ %s", COLOR_BRIGHT_GREEN, COLOR_RESET);
                        printf("%s\n", skins[listing_idx].name);
                        printf("Wear: %s\n", wear_to_string(skins[listing_idx].wear));
                        printf("Pattern: #%d\n", skins[listing_idx].pattern_seed);
                        printf("Price: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, listings[listing_idx].price, COLOR_RESET);
                        printf("\n");
                        display_balance_info();
                        printf("\n");
                        print_separator(50);
                        printf("Purchase this item? (y/n): ");
                        fflush(stdout);

                        char confirm[32];
                        if (fgets(confirm, sizeof(confirm), stdin) == NULL)
                            continue;

                        if (confirm[0] != 'y' && confirm[0] != 'Y')
                            continue;

                        // Buy from market
                        memset(&request, 0, sizeof(Message));
                        memset(&response, 0, sizeof(Message));
                        request.header.magic = 0xABCD;
                        request.header.msg_type = MSG_BUY_FROM_MARKET;
                        snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, listings[listing_idx].listing_id);
                        request.header.msg_length = strlen(request.payload);

                        if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                        {
                            if (response.header.msg_type == MSG_BUY_FROM_MARKET)
                            {
                                clear_screen();
                                print_header("PURCHASE SUCCESS");
                                print_success("Item purchased successfully!");
                                printf("\n");
                                // Display purchased item details
                                const char *rarity_color = get_rarity_color(skins[listing_idx].rarity);
                                const char *stattrak = skins[listing_idx].is_stattrak ? "StatTrak™ " : "";
                                const char *wear = wear_to_string(skins[listing_idx].wear);
                                printf("Item: %s[%s]%s ", rarity_color, rarity_to_string(skins[listing_idx].rarity), COLOR_RESET);
                                if (skins[listing_idx].is_stattrak)
                                    printf("%sStatTrak™ %s", COLOR_BRIGHT_GREEN, COLOR_RESET);
                                printf("%s\n", skins[listing_idx].name);
                                printf("Wear: %s\n", wear);
                                printf("Pattern: #%d\n", skins[listing_idx].pattern_seed);
                                printf("Price: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, listings[listing_idx].price, COLOR_RESET);
                                printf("\n%sAdded to your inventory!%s\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
                                printf("\nPress Enter to continue...");
                                getchar();
                                // Loop will continue and refresh the market list
                            }
                            else if (response.header.msg_type == MSG_ERROR)
                            {
                                uint32_t error_code;
                                memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));

                                clear_screen();
                                print_header("PURCHASE FAILED");
                                if (error_code == ERR_INSUFFICIENT_FUNDS)
                                    print_error("Insufficient funds");
                                else if (error_code == ERR_ITEM_NOT_FOUND)
                                    print_error("Listing not found or already sold");
                                else if (error_code == ERR_PERMISSION_DENIED)
                                    print_error("Cannot buy your own listing");
                                else
                                    print_error("Failed to purchase item");
                                printf("\nPress Enter to continue...");
                                getchar();
                            }
                        }
                        else
                        {
                            print_error("Failed to communicate with server");
                            sleep(2);
                        }
                    }
                    else
                    {
                        print_error("Invalid listing number");
                        sleep(1);
                    }
                }
            }
        }
        else
        {
            print_error("Failed to load market listings");
            wait_for_key();
            return;
        }
    }
}

void show_unbox()
{
    int should_exit = 0;

    while (!should_exit)
    {
        clear_screen();
        print_header("UNBOX CASES");
        display_balance_info();

        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        // Get available cases
        request.header.magic = 0xABCD;
        request.header.msg_type = MSG_GET_CASES;
        request.header.msg_length = 0;

        if (send_message_to_server(&request) != 0)
        {
            print_error("Failed to request cases");
            wait_for_key();
            return;
        }

        if (receive_message_from_server(&response) != 0)
        {
            print_error("Failed to receive cases");
            wait_for_key();
            return;
        }

        if (response.header.msg_type == MSG_CASES_DATA)
        {
            Case cases[50];
            memset(cases, 0, sizeof(cases)); // Clear array first
            int count = response.header.msg_length / sizeof(Case);
            if (count > 50)
                count = 50;

            memcpy(cases, response.payload, count * sizeof(Case));
            
            // Ensure all name fields are null-terminated
            for (int i = 0; i < count; i++)
            {
                cases[i].name[sizeof(cases[i].name) - 1] = '\0';
            }

            printf("\nAvailable Cases:\n\n");
            for (int i = 0; i < count; i++)
            {
                printf("%d. %s - $%.2f\n", i + 1, cases[i].name, cases[i].price);
            }

            printf("\nSelect case to unbox (1-%d, 0 to cancel): ", count);
            fflush(stdout);

            char input[32];
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                int choice = atoi(input);
                if (choice == 0)
                {
                    should_exit = 1;
                    break;
                }
                else if (choice > 0 && choice <= count)
                {
                    // Unbox case
                    request.header.magic = 0xABCD;
                    request.header.msg_type = MSG_UNBOX_CASE;
                    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, cases[choice - 1].case_id);
                    request.header.msg_length = strlen(request.payload);

                    if (send_message_to_server(&request) == 0)
                    {
                        if (receive_message_from_server(&response) == 0)
                        {
                            if (response.header.msg_type == MSG_UNBOX_RESULT)
                            {
                                if (response.header.msg_length >= sizeof(Skin))
                                {
                                    Skin unboxed;
                                    memset(&unboxed, 0, sizeof(Skin)); // Clear first
                                    memcpy(&unboxed, response.payload, sizeof(Skin));
                                    // Ensure name is null-terminated
                                    unboxed.name[sizeof(unboxed.name) - 1] = '\0';

                                    // Spinning animation: items scrolling fast then slowing down
                                    clear_screen();
                                    printf("\n\n");

                                    // Simulate spinning through items (fast -> slow)
                                    const char *spinner_chars = "|/-\\";
                                    const char *rarity_names[] = {"Consumer", "Industrial", "Mil-Spec", "Restricted", "Classified", "Covert", "Contraband"};
                                    const char *rarity_colors[] = {COLOR_BLUE, COLOR_CYAN, COLOR_MAGENTA, COLOR_MAGENTA, COLOR_RED, COLOR_YELLOW, COLOR_BRIGHT_YELLOW};

                                    // Fast spin phase (20 iterations, 50ms each = 1 second)
                                    for (int i = 0; i < 20; i++)
                                    {
                                        int rarity_idx = i % 7;
                                        printf("\r  %s[%s]%s %s%s%s  %c  ",
                                               rarity_colors[rarity_idx],
                                               rarity_names[rarity_idx],
                                               COLOR_RESET,
                                               COLOR_DIM,
                                               "Spinning...",
                                               COLOR_RESET,
                                               spinner_chars[i % 4]);
                                        fflush(stdout);
                                        usleep(50000); // 50ms - fast
                                    }

                                    // Slow spin phase (10 iterations, 150ms each = 1.5 seconds)
                                    for (int i = 0; i < 10; i++)
                                    {
                                        int rarity_idx = (20 + i) % 7;
                                        printf("\r  %s[%s]%s %s%s%s  %c  ",
                                               rarity_colors[rarity_idx],
                                               rarity_names[rarity_idx],
                                               COLOR_RESET,
                                               COLOR_DIM,
                                               "Slowing down...",
                                               COLOR_RESET,
                                               spinner_chars[(20 + i) % 4]);
                                        fflush(stdout);
                                        usleep(150000); // 150ms - slower
                                    }

                                    // Final reveal (show actual rarity briefly before result)
                                    const char *final_rarity_color = get_rarity_color(unboxed.rarity);
                                    const char *final_rarity_name = rarity_to_string(unboxed.rarity);
                                    printf("\r  %s[%s]%s %s%s%s  *  ",
                                           final_rarity_color,
                                           final_rarity_name,
                                           COLOR_RESET,
                                           COLOR_BRIGHT_GREEN,
                                           "REVEALING...",
                                           COLOR_RESET);
                                    fflush(stdout);
                                    usleep(500000); // 500ms pause before reveal

                                    // Display unboxed skin
                                    clear_screen();
                                    printf("\n\n");
                                    print_box(20, 3, 60, 12, "UNBOXED!");
                                    move_cursor(5, 22);
                                    print_skin(&unboxed, 22, 5);
                                    move_cursor(11, 22);
                                    print_success("Case opened successfully!");
                                    printf("\n\n");
                                    wait_for_key();
                                    // Loop will continue, showing case selection again
                                }
                                else
                                {
                                    print_error("Invalid response size from server");
                                    wait_for_key();
                                }
                            }
                            else if (response.header.msg_type == MSG_ERROR)
                            {
                                uint32_t error_code;
                                memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));

                                if (error_code == ERR_INSUFFICIENT_FUNDS)
                                    print_error("Insufficient funds to open case");
                                else if (error_code == ERR_INVALID_REQUEST)
                                    print_error("Invalid case selection");
                                else if (error_code == ERR_ITEM_NOT_FOUND)
                                    print_error("Case or user not found");
                                else if (error_code == ERR_DATABASE_ERROR)
                                    print_error("Database error: Unable to process unbox request");
                                else
                                {
                                    char err_msg[128];
                                    snprintf(err_msg, sizeof(err_msg), "Unbox failed: error code %u", error_code);
                                    print_error(err_msg);
                                }
                                wait_for_key();
                            }
                            else
                            {
                                print_error("Failed to unbox case");
                                wait_for_key();
                            }
                        }
                        else
                        {
                            print_error("Failed to receive unbox response");
                            wait_for_key();
                        }
                    }
                    else
                    {
                        print_error("Failed to send unbox request");
                        wait_for_key();
                    }
                }
            }
        }
        else
        {
            print_error("Failed to load cases");
            wait_for_key();
            break;
        }
    } // End while loop
}

// Show leaderboards
void show_leaderboards()
{
    int should_exit = 0;
    while (!should_exit)
    {
        clear_screen();
        print_header("LEADERBOARDS");
        display_balance_info();

        printf("\nOptions:\n");
        printf("1. Top Traders (by Net Worth)\n");
        printf("2. Luckiest Unboxers\n");
        printf("3. Most Profitable\n");
        printf("0. Back to main menu\n");
        printf("\nSelect option: ");
        fflush(stdout);

        char choice[32];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            should_exit = 1;
            break;
        }

        int option = atoi(choice);
        if (option == 0)
        {
            should_exit = 1;
            break;
        }

        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        uint16_t msg_type = 0;
        uint16_t response_type = 0;
        const char *title = "";

        if (option == 1)
        {
            msg_type = MSG_GET_TOP_TRADERS;
            response_type = MSG_TOP_TRADERS_DATA;
            title = "TOP TRADERS (BY NET WORTH)";
        }
        else if (option == 2)
        {
            msg_type = MSG_GET_LUCKIEST_UNBOXERS;
            response_type = MSG_LUCKIEST_UNBOXERS_DATA;
            title = "LUCKIEST UNBOXERS";
        }
        else if (option == 3)
        {
            msg_type = MSG_GET_MOST_PROFITABLE;
            response_type = MSG_MOST_PROFITABLE_DATA;
            title = "MOST PROFITABLE";
        }
        else
        {
            print_error("Invalid option");
            sleep(1);
            continue;
        }

        request.header.magic = 0xABCD;
        request.header.msg_type = msg_type;
        snprintf(request.payload, MAX_PAYLOAD_SIZE, "10"); // Limit 10
        request.header.msg_length = strlen(request.payload);

        if (send_message_to_server(&request) != 0)
        {
            print_error("Failed to request leaderboard");
            wait_for_key();
            continue;
        }

        if (receive_message_from_server(&response) != 0)
        {
            print_error("Failed to receive leaderboard");
            wait_for_key();
            continue;
        }

        if (response.header.msg_type == response_type)
        {
            clear_screen();
            print_header(title);
            printf("\n");

            int count = response.header.msg_length / sizeof(LeaderboardEntry);
            if (count > 0)
            {
                LeaderboardEntry entries[100];
                memcpy(entries, response.payload, sizeof(LeaderboardEntry) * count);

                printf("%sRank  Username                    Value           Details%s\n", COLOR_CYAN, COLOR_RESET);
                print_separator(70);

                for (int i = 0; i < count; i++)
                {
                    const char *medal = "";
                    if (i == 0)
                        medal = "🥇";
                    else if (i == 1)
                        medal = "🥈";
                    else if (i == 2)
                        medal = "🥉";

                    printf("%s%2d%s. %s%-28s%s %s$%10.2f%s",
                           COLOR_BRIGHT_GREEN, i + 1, COLOR_RESET,
                           COLOR_CYAN, entries[i].username, COLOR_RESET,
                           COLOR_BRIGHT_GREEN, entries[i].value, COLOR_RESET);

                    if (strlen(entries[i].details) > 0)
                    {
                        printf(" %s%s%s", COLOR_DIM, entries[i].details, COLOR_RESET);
                    }
                    printf(" %s\n", medal);
                }
            }
            else
            {
                printf("No data available.\n");
            }

            printf("\nPress Enter to continue...");
            getchar();
        }
        else if (response.header.msg_type == MSG_ERROR)
        {
            uint32_t error_code;
            memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
            char err_msg[128];
            snprintf(err_msg, sizeof(err_msg), "Failed to load leaderboard (error: %u)", error_code);
            print_error(err_msg);
            wait_for_key();
        }
        else
        {
            print_error("Failed to load leaderboard (unexpected response)");
            wait_for_key();
        }
    }
}

// Show trade analytics
void show_trade_analytics()
{
    int should_exit = 0;
    while (!should_exit)
    {
        clear_screen();
        print_header("TRADE ANALYTICS");
        display_balance_info();

        printf("\nOptions:\n");
        printf("1. Trade History\n");
        printf("2. Trade Statistics\n");
        printf("3. Balance History Graph\n");
        printf("0. Back to main menu\n");
        printf("\nSelect option: ");
        fflush(stdout);

        char choice[32];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            should_exit = 1;
            break;
        }

        int option = atoi(choice);
        if (option == 0)
        {
            should_exit = 1;
            break;
        }

        if (option == 1)
        {
            // Trade History
            Message request, response;
            memset(&request, 0, sizeof(Message));
            memset(&response, 0, sizeof(Message));

            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_GET_TRADE_HISTORY;
            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:50", g_user_id);
            request.header.msg_length = strlen(request.payload);

            if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
            {
                if (response.header.msg_type == MSG_TRADE_HISTORY_DATA)
                {
                    clear_screen();
                    print_header("TRADE HISTORY");
                    printf("\n");

                    int count = response.header.msg_length / sizeof(TransactionLog);
                    if (count > 0)
                    {
                        TransactionLog logs[100];
                        memcpy(logs, response.payload, sizeof(TransactionLog) * count);

                        for (int i = 0; i < count; i++)
                        {
                            const char *type_str = "";
                            const char *type_color = COLOR_DIM;
                            switch (logs[i].type)
                            {
                            case LOG_MARKET_BUY:
                                type_str = "BUY";
                                type_color = COLOR_BRIGHT_RED;
                                break;
                            case LOG_MARKET_SELL:
                                type_str = "SELL";
                                type_color = COLOR_BRIGHT_GREEN;
                                break;
                            case LOG_TRADE:
                                type_str = "TRADE";
                                type_color = COLOR_CYAN;
                                break;
                            default:
                                type_str = "OTHER";
                                break;
                            }

                            struct tm *timeinfo = localtime(&logs[i].timestamp);
                            char time_str[64];
                            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

                            printf("[%s] %s%s%s: %s\n", time_str, type_color, type_str, COLOR_RESET, logs[i].details);
                        }
                    }
                    else
                    {
                        printf("No trade history available.\n");
                    }

                    printf("\nPress Enter to continue...");
                    getchar();
                }
            }
        }
        else if (option == 2)
        {
            // Trade Statistics
            Message request, response;
            memset(&request, 0, sizeof(Message));
            memset(&response, 0, sizeof(Message));

            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_GET_TRADE_STATS;
            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
            request.header.msg_length = strlen(request.payload);

            if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
            {
                if (response.header.msg_type == MSG_TRADE_STATS_DATA)
                {
                    clear_screen();
                    print_header("TRADE STATISTICS");
                    printf("\n");

                    TradeStats stats;
                    memcpy(&stats, response.payload, sizeof(TradeStats));

                    printf("Trades Completed: %s%d%s\n", COLOR_CYAN, stats.trades_completed, COLOR_RESET);
                    printf("Items Bought: %s%d%s\n", COLOR_BRIGHT_RED, stats.items_bought, COLOR_RESET);
                    printf("Items Sold: %s%d%s\n", COLOR_BRIGHT_GREEN, stats.items_sold, COLOR_RESET);
                    printf("Average Buy Price: %s$%.2f%s\n", COLOR_BRIGHT_RED, stats.avg_buy_price, COLOR_RESET);
                    printf("Average Sell Price: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, stats.avg_sell_price, COLOR_RESET);
                    printf("Net Profit: %s$%.2f%s\n",
                           stats.net_profit >= 0 ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED,
                           stats.net_profit, COLOR_RESET);
                    printf("Best Trade Profit: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, stats.best_trade_profit, COLOR_RESET);
                    printf("Worst Trade Loss: %s$%.2f%s\n", COLOR_BRIGHT_RED, stats.worst_trade_loss, COLOR_RESET);
                    printf("Win Rate: %s%.1f%%%s\n",
                           stats.win_rate >= 50.0f ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED,
                           stats.win_rate, COLOR_RESET);

                    printf("\nPress Enter to continue...");
                    getchar();
                }
            }
        }
        else if (option == 3)
        {
            // Balance History Graph
            Message request, response;
            memset(&request, 0, sizeof(Message));
            memset(&response, 0, sizeof(Message));

            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_GET_BALANCE_HISTORY;
            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:7", g_user_id);
            request.header.msg_length = strlen(request.payload);

            if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
            {
                if (response.header.msg_type == MSG_BALANCE_HISTORY_DATA)
                {
                    clear_screen();
                    print_header("BALANCE HISTORY (Last 7 Days)");
                    printf("\n");

                    int count = response.header.msg_length / sizeof(BalanceHistoryEntry);
                    if (count > 0)
                    {
                        BalanceHistoryEntry history[30];
                        memcpy(history, response.payload, sizeof(BalanceHistoryEntry) * count);

                        // Find min and max for scaling
                        float min_balance = history[0].balance;
                        float max_balance = history[0].balance;
                        for (int i = 1; i < count; i++)
                        {
                            if (history[i].balance < min_balance)
                                min_balance = history[i].balance;
                            if (history[i].balance > max_balance)
                                max_balance = history[i].balance;
                        }
                        float range = max_balance - min_balance;
                        if (range < 1.0f)
                            range = 1.0f;

                        // Simple ASCII graph
                        int graph_width = 60;
                        int graph_height = 15;

                        printf("Balance Range: $%.2f - $%.2f\n\n", min_balance, max_balance);

                        for (int row = graph_height - 1; row >= 0; row--)
                        {
                            float value = min_balance + (range * row / graph_height);
                            printf("%6.0f │", value);

                            for (int col = 0; col < count && col < graph_width; col++)
                            {
                                float normalized = (history[col].balance - min_balance) / range;
                                int bar_height = (int)(normalized * graph_height);

                                if (bar_height == row)
                                    printf("█");
                                else if (bar_height > row)
                                    printf("│");
                                else
                                    printf(" ");
                            }
                            printf("\n");
                        }

                        printf("       └");
                        for (int i = 0; i < graph_width && i < count; i++)
                            printf("─");
                        printf("\n");

                        // Print dates
                        printf("        ");
                        for (int i = 0; i < count && i < 5; i++)
                        {
                            struct tm *timeinfo = localtime(&history[i].timestamp);
                            char date_str[16];
                            strftime(date_str, sizeof(date_str), "%m/%d", timeinfo);
                            printf("%-12s", date_str);
                        }
                        printf("\n");
                    }
                    else
                    {
                        printf("No balance history available.\n");
                    }

                    printf("\nPress Enter to continue...");
                    getchar();
                }
            }
        }
        else
        {
            print_error("Invalid option");
            sleep(1);
        }
    }
}

// Show trading challenges
void show_trading_challenges()
{
    int should_exit = 0;
    TradingChallenge challenges[50];
    int challenge_count = 0;

    while (!should_exit)
    {
        clear_screen();
        print_header("TRADING CHALLENGES");
        display_balance_info();

        // Get user's challenges
        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        request.header.magic = 0xABCD;
        request.header.msg_type = MSG_GET_USER_CHALLENGES;
        snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
        request.header.msg_length = strlen(request.payload);

        challenge_count = 0;

        if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
        {
            if (response.header.msg_type == MSG_USER_CHALLENGES_DATA)
            {
                challenge_count = response.header.msg_length / sizeof(TradingChallenge);
                if (challenge_count > 50)
                    challenge_count = 50;
                if (challenge_count > 0)
                {
                    memcpy(challenges, response.payload, sizeof(TradingChallenge) * challenge_count);
                }
            }
        }

        if (challenge_count > 0)
        {
            printf("\n%sActive Challenges:%s\n", COLOR_CYAN, COLOR_RESET);
            print_separator(70);

            for (int i = 0; i < challenge_count; i++)
            {
                const char *status_str = "";
                const char *status_color = COLOR_DIM;
                switch (challenges[i].status)
                {
                case 0: // PENDING
                    status_str = "PENDING";
                    status_color = COLOR_YELLOW;
                    break;
                case 1: // ACTIVE
                    status_str = "ACTIVE";
                    status_color = COLOR_BRIGHT_GREEN;
                    break;
                case 2: // COMPLETED
                    status_str = "COMPLETED";
                    status_color = COLOR_CYAN;
                    break;
                case 3: // CANCELLED
                    status_str = "CANCELLED";
                    status_color = COLOR_BRIGHT_RED;
                    break;
                }

                // Get challenger and opponent usernames via API
                char challenger_name[32] = "User";
                char opponent_name[32] = "User";

                Message req1, resp1, req2, resp2;
                memset(&req1, 0, sizeof(Message));
                memset(&resp1, 0, sizeof(Message));
                memset(&req2, 0, sizeof(Message));
                memset(&resp2, 0, sizeof(Message));

                req1.header.magic = 0xABCD;
                req1.header.msg_type = MSG_GET_USER_PROFILE;
                snprintf(req1.payload, MAX_PAYLOAD_SIZE, "%d", challenges[i].challenger_id);
                req1.header.msg_length = strlen(req1.payload);

                req2.header.magic = 0xABCD;
                req2.header.msg_type = MSG_GET_USER_PROFILE;
                snprintf(req2.payload, MAX_PAYLOAD_SIZE, "%d", challenges[i].opponent_id);
                req2.header.msg_length = strlen(req2.payload);

                if (send_message_to_server(&req1) == 0 && receive_message_from_server(&resp1) == 0)
                {
                    if (resp1.header.msg_type == MSG_USER_PROFILE_DATA)
                    {
                        User u;
                        memcpy(&u, resp1.payload, sizeof(User));
                        strncpy(challenger_name, u.username, 31);
                    }
                }

                if (send_message_to_server(&req2) == 0 && receive_message_from_server(&resp2) == 0)
                {
                    if (resp2.header.msg_type == MSG_USER_PROFILE_DATA)
                    {
                        User u;
                        memcpy(&u, resp2.payload, sizeof(User));
                        strncpy(opponent_name, u.username, 31);
                    }
                }

                printf("Challenge #%d: %s vs %s [%s%s%s]\n",
                       challenges[i].challenge_id,
                       challenger_name, opponent_name,
                       status_color, status_str, COLOR_RESET);
                
                // Determine which profit is "yours" vs "opponent"
                float your_profit, opponent_profit;
                if (challenges[i].challenger_id == g_user_id)
                {
                    your_profit = challenges[i].challenger_current_profit;
                    opponent_profit = challenges[i].opponent_current_profit;
                }
                else
                {
                    your_profit = challenges[i].opponent_current_profit;
                    opponent_profit = challenges[i].challenger_current_profit;
                }
                
                printf("  Your Profit: %s$%.2f%s | Opponent Profit: %s$%.2f%s\n",
                       COLOR_BRIGHT_GREEN, your_profit, COLOR_RESET,
                       COLOR_BRIGHT_RED, opponent_profit, COLOR_RESET);
                printf("  Duration: %d minutes\n", challenges[i].duration_minutes);
                
                // Display timer countdown for active challenges
                if (challenges[i].status == CHALLENGE_ACTIVE && challenges[i].start_time > 0)
                {
                    time_t now = time(NULL);
                    time_t elapsed = now - challenges[i].start_time;
                    int total_seconds = challenges[i].duration_minutes * 60;
                    int remaining_seconds = total_seconds - (int)elapsed;
                    
                    if (remaining_seconds > 0)
                    {
                        int minutes = remaining_seconds / 60;
                        int seconds = remaining_seconds % 60;
                        printf("  Timer: %s%02d:%02d remaining%s\n", 
                               COLOR_YELLOW, minutes, seconds, COLOR_RESET);
                    }
                    else
                    {
                        printf("  Timer: %sEXPIRED%s\n", COLOR_BRIGHT_RED, COLOR_RESET);
                    }
                }
            }
        }
        else
        {
            printf("\nNo active challenges.\n");
        }

        printf("\nOptions:\n");
        printf("1. Create Challenge\n");
        printf("2. Accept Challenge\n");
        printf("3. Update Progress\n");
        printf("4. Complete Challenge\n");
        printf("5. Cancel Challenge\n");
        printf("0. Back to main menu\n");
        printf("\nSelect option: ");
        fflush(stdout);

        char choice[32];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            should_exit = 1;
            break;
        }

        int option = atoi(choice);
        if (option == 0)
        {
            should_exit = 1;
            break;
        }
        else if (option == 1)
        {
            // Create challenge
            printf("Enter opponent username: ");
            fflush(stdout);
            char username[32];
            if (fgets(username, sizeof(username), stdin) != NULL)
            {
                size_t len = strlen(username);
                if (len > 0 && username[len - 1] == '\n')
                    username[len - 1] = '\0';

                User opponent;
                if (search_user_by_username(username, &opponent) == 0)
                {
                    printf("Enter duration (minutes): ");
                    fflush(stdout);
                    char duration_str[32];
                    if (fgets(duration_str, sizeof(duration_str), stdin) != NULL)
                    {
                        int duration = atoi(duration_str);
                        if (duration > 0)
                        {
                            Message req, resp;
                            memset(&req, 0, sizeof(Message));
                            memset(&resp, 0, sizeof(Message));

                            req.header.magic = 0xABCD;
                            req.header.msg_type = MSG_CREATE_CHALLENGE;
                            snprintf(req.payload, MAX_PAYLOAD_SIZE, "%d:%d:%d", g_user_id, opponent.user_id, duration);
                            req.header.msg_length = strlen(req.payload);

                            if (send_message_to_server(&req) == 0 && receive_message_from_server(&resp) == 0)
                            {
                                if (resp.header.msg_type == MSG_CREATE_CHALLENGE_RESPONSE)
                                {
                                    int challenge_id = 0;
                                    memcpy(&challenge_id, resp.payload, sizeof(int));
                                    char msg[128];
                                    snprintf(msg, sizeof(msg), "Challenge created! ID: %d", challenge_id);
                                    print_success(msg);
                                }
                                else
                                {
                                    print_error("Failed to create challenge");
                                }
                            }
                        }
                    }
                }
                else
                {
                    print_error("User not found");
                }
            }
            wait_for_key();
        }
        else if (option == 2)
        {
            // Accept Challenge
            if (challenge_count == 0)
            {
                print_error("No challenges available");
                wait_for_key();
                continue;
            }
            
            printf("\nEnter challenge ID to accept: ");
            fflush(stdout);
            char challenge_id_str[32];
            if (fgets(challenge_id_str, sizeof(challenge_id_str), stdin) != NULL)
            {
                int challenge_id = atoi(challenge_id_str);
                
                Message req, resp;
                memset(&req, 0, sizeof(Message));
                memset(&resp, 0, sizeof(Message));
                
                req.header.magic = 0xABCD;
                req.header.msg_type = MSG_ACCEPT_CHALLENGE;
                snprintf(req.payload, MAX_PAYLOAD_SIZE, "%d:%d", challenge_id, g_user_id);
                req.header.msg_length = strlen(req.payload);
                
                if (send_message_to_server(&req) == 0 && receive_message_from_server(&resp) == 0)
                {
                    if (resp.header.msg_type == MSG_ACCEPT_CHALLENGE_RESPONSE)
                    {
                        print_success("Challenge accepted!");
                    }
                    else if (resp.header.msg_type == MSG_ERROR)
                    {
                        uint32_t error_code;
                        memcpy(&error_code, resp.payload + sizeof(uint16_t), sizeof(uint32_t));
                        if (error_code == ERR_PERMISSION_DENIED)
                            print_error("You are not authorized to accept this challenge");
                        else if (error_code == ERR_INVALID_REQUEST)
                            print_error("Challenge is not pending or not found");
                        else
                            print_error("Failed to accept challenge");
                    }
                }
            }
            wait_for_key();
        }
        else if (option == 3)
        {
            // Update Progress
            if (challenge_count == 0)
            {
                print_error("No challenges available");
            wait_for_key();
                continue;
            }
            
            printf("\nEnter challenge ID to update: ");
            fflush(stdout);
            char challenge_id_str[32];
            if (fgets(challenge_id_str, sizeof(challenge_id_str), stdin) != NULL)
            {
                int challenge_id = atoi(challenge_id_str);
                
                Message req, resp;
                memset(&req, 0, sizeof(Message));
                memset(&resp, 0, sizeof(Message));
                
                req.header.magic = 0xABCD;
                req.header.msg_type = MSG_UPDATE_CHALLENGE_PROGRESS;
                snprintf(req.payload, MAX_PAYLOAD_SIZE, "%d", challenge_id);
                req.header.msg_length = strlen(req.payload);
                
                if (send_message_to_server(&req) == 0 && receive_message_from_server(&resp) == 0)
                {
                    if (resp.header.msg_type == MSG_UPDATE_CHALLENGE_PROGRESS_RESPONSE)
                    {
                        TradingChallenge updated_challenge;
                        memcpy(&updated_challenge, resp.payload, sizeof(TradingChallenge));
                        
                        float your_profit, opponent_profit;
                        if (updated_challenge.challenger_id == g_user_id)
                        {
                            your_profit = updated_challenge.challenger_current_profit;
                            opponent_profit = updated_challenge.opponent_current_profit;
                        }
                        else
                        {
                            your_profit = updated_challenge.opponent_current_profit;
                            opponent_profit = updated_challenge.challenger_current_profit;
                        }
                        
                        printf("\n%sProgress Updated:%s\n", COLOR_CYAN, COLOR_RESET);
                        printf("Your Profit: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, your_profit, COLOR_RESET);
                        printf("Opponent Profit: %s$%.2f%s\n", COLOR_BRIGHT_RED, opponent_profit, COLOR_RESET);
                        
                        if (your_profit > opponent_profit)
                            printf("%sYou are winning!%s\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
                        else if (opponent_profit > your_profit)
                            printf("%sYou are losing! Trade smarter!%s\n", COLOR_BRIGHT_RED, COLOR_RESET);
                        else
                            printf("%sTied!%s\n", COLOR_YELLOW, COLOR_RESET);
                    }
                    else
                    {
                        print_error("Failed to update challenge progress");
                    }
                }
            }
            wait_for_key();
        }
        else if (option == 4)
        {
            // Complete Challenge
            if (challenge_count == 0)
            {
                print_error("No challenges available");
                wait_for_key();
                continue;
            }
            
            printf("\nEnter challenge ID to complete: ");
            fflush(stdout);
            char challenge_id_str[32];
            if (fgets(challenge_id_str, sizeof(challenge_id_str), stdin) != NULL)
            {
                int challenge_id = atoi(challenge_id_str);
                
                Message req, resp;
                memset(&req, 0, sizeof(Message));
                memset(&resp, 0, sizeof(Message));
                
                req.header.magic = 0xABCD;
                req.header.msg_type = MSG_COMPLETE_CHALLENGE;
                snprintf(req.payload, MAX_PAYLOAD_SIZE, "%d", challenge_id);
                req.header.msg_length = strlen(req.payload);
                
                if (send_message_to_server(&req) == 0 && receive_message_from_server(&resp) == 0)
                {
                    if (resp.header.msg_type == MSG_COMPLETE_CHALLENGE_RESPONSE)
                    {
                        int winner_id = 0;
                        memcpy(&winner_id, resp.payload, sizeof(int));
                        
                        if (winner_id == 0)
                        {
                            print_success("Challenge completed! It's a tie!");
                        }
                        else if (winner_id == g_user_id)
                        {
                            print_success("Challenge completed! You won!");
                        }
                        else
                        {
                            print_error("Challenge completed! You lost!");
                        }
                    }
                    else
                    {
                        print_error("Failed to complete challenge");
                    }
                }
            }
            wait_for_key();
        }
        else if (option == 5)
        {
            // Cancel Challenge
            if (challenge_count == 0)
            {
                print_error("No challenges available");
                wait_for_key();
                continue;
            }
            
            printf("\nEnter challenge ID to cancel: ");
            fflush(stdout);
            char challenge_id_str[32];
            if (fgets(challenge_id_str, sizeof(challenge_id_str), stdin) != NULL)
            {
                int challenge_id = atoi(challenge_id_str);
                
                Message req, resp;
                memset(&req, 0, sizeof(Message));
                memset(&resp, 0, sizeof(Message));
                
                req.header.magic = 0xABCD;
                req.header.msg_type = MSG_CANCEL_CHALLENGE;
                snprintf(req.payload, MAX_PAYLOAD_SIZE, "%d:%d", challenge_id, g_user_id);
                req.header.msg_length = strlen(req.payload);
                
                if (send_message_to_server(&req) == 0 && receive_message_from_server(&resp) == 0)
                {
                    if (resp.header.msg_type == MSG_CANCEL_CHALLENGE_RESPONSE)
                    {
                        print_success("Challenge cancelled");
                    }
                    else if (resp.header.msg_type == MSG_ERROR)
                    {
                        uint32_t error_code;
                        memcpy(&error_code, resp.payload + sizeof(uint16_t), sizeof(uint32_t));
                        if (error_code == ERR_PERMISSION_DENIED)
                            print_error("You are not authorized to cancel this challenge");
                        else if (error_code == ERR_INVALID_REQUEST)
                            print_error("Cannot cancel completed challenge");
                        else
                            print_error("Failed to cancel challenge");
                    }
                }
            }
            wait_for_key();
        }
        else
        {
            print_error("Invalid option");
            wait_for_key();
        }
    }
}

void show_profile()
{
    int should_exit = 0;
    while (!should_exit)
    {
        clear_screen();
        print_header("PROFILE");
        display_balance_info();

        printf("\nOptions:\n");
        printf("1. View my profile\n");
        printf("2. Search user by username\n");
        printf("0. Back to main menu\n");
        printf("Select option: ");
        fflush(stdout);

        char choice[32];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            should_exit = 1;
            break;
        }

        int option = atoi(choice);
        if (option == 0)
        {
            should_exit = 1;
            break;
        }
        else if (option == 1)
        {
            // View own profile
            Message request, response;
            memset(&request, 0, sizeof(Message));
            memset(&response, 0, sizeof(Message));

            request.header.magic = 0xABCD;
            request.header.msg_type = MSG_GET_USER_PROFILE;
            snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
            request.header.msg_length = strlen(request.payload);

            if (send_message_to_server(&request) == 0)
            {
                if (receive_message_from_server(&response) == 0)
                {
                    if (response.header.msg_type == MSG_USER_PROFILE_DATA)
                    {
                        User user;
                        memcpy(&user, response.payload, sizeof(User));

                        clear_screen();
                        print_header("MY PROFILE");
                        display_balance_info();

                        printf("\n");
                        print_box(10, 5, 60, 15, "User Profile");
                        move_cursor(7, 12);
                        printf("Username: %s%s%s\n", STYLE_BOLD, user.username, COLOR_RESET);
                        move_cursor(8, 12);
                        printf("Balance: %s$%.2f%s\n", COLOR_GREEN, user.balance, COLOR_RESET);
                        move_cursor(9, 12);
                        printf("User ID: %d\n", user.user_id);
                        move_cursor(10, 12);
                        float inv_value = calculate_inventory_value();
                        printf("Inventory Value: %s$%.2f%s\n", COLOR_YELLOW, inv_value, COLOR_RESET);
                        move_cursor(11, 12);
                        printf("Total Value: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, user.balance + inv_value, COLOR_RESET);

                        wait_for_key();
                    }
                }
            }
        }
        else if (option == 2)
        {
            // Search user by username
            printf("\nEnter username to search: ");
            fflush(stdout);
            char username[64];
            if (fgets(username, sizeof(username), stdin) == NULL)
                continue;

            // Remove newline
            size_t len = strlen(username);
            if (len > 0 && username[len - 1] == '\n')
                username[len - 1] = '\0';

            if (strlen(username) == 0)
            {
                print_error("Username cannot be empty");
                sleep(2);
                continue;
            }

            User found_user;
            int result = search_user_by_username(username, &found_user);

            if (result == 0)
            {
                clear_screen();
                print_header("USER PROFILE");
                display_balance_info();

                printf("\n");
                print_box(10, 5, 60, 18, "User Profile");
                move_cursor(7, 12);
                printf("Username: %s%s%s\n", STYLE_BOLD, found_user.username, COLOR_RESET);
                move_cursor(8, 12);
                printf("Balance: %s$%.2f%s\n", COLOR_GREEN, found_user.balance, COLOR_RESET);
                move_cursor(9, 12);
                printf("User ID: %d\n", found_user.user_id);

                if (found_user.user_id == g_user_id)
                {
                    move_cursor(10, 12);
                    float inv_value = calculate_inventory_value();
                    printf("Inventory Value: %s$%.2f%s\n", COLOR_YELLOW, inv_value, COLOR_RESET);
                    move_cursor(11, 12);
                    printf("Total Value: %s$%.2f%s\n", COLOR_BRIGHT_GREEN, found_user.balance + inv_value, COLOR_RESET);
                }

                printf("\n");
                print_separator(50);
                if (found_user.user_id != g_user_id)
                {
                    printf("Options:\n");
                    printf("1. Send trade offer\n");
                    printf("0. Back\n");
                    printf("Select option: ");
                    fflush(stdout);

                    char trade_choice[32];
                    if (fgets(trade_choice, sizeof(trade_choice), stdin) != NULL)
                    {
                        int trade_option = atoi(trade_choice);
                        if (trade_option == 1)
                        {
                            // Send trade offer to this user
                            send_trade_offer_ui(found_user.user_id, found_user.username);
                        }
                    }
                }
                else
                {
                    wait_for_key();
                }
            }
            else
            {
                print_error("User not found");
                sleep(2);
            }
        }
        else
        {
            print_error("Invalid option");
            sleep(1);
        }
    }
}

// Helper function to send trade offer
static void send_trade_offer_ui(int to_user_id, const char *to_username)
{
    clear_screen();
    print_header("SEND TRADE OFFER");
    display_balance_info();

    printf("\nTrading with: %s%s%s (User ID: %d)\n\n", STYLE_BOLD, to_username, COLOR_RESET, to_user_id);

    // Load current user's inventory
    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_INVENTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) != 0 || receive_message_from_server(&response) != 0)
    {
        print_error("Failed to load inventory");
        wait_for_key();
        return;
    }

    if (response.header.msg_type != MSG_INVENTORY_DATA)
    {
        print_error("Failed to load inventory");
        wait_for_key();
        return;
    }

    Inventory inv;
    memcpy(&inv, response.payload, sizeof(Inventory));

    if (inv.count == 0)
    {
        print_error("Your inventory is empty. Cannot send trade offer.");
        wait_for_key();
        return;
    }

    // Load all skin details
    Skin skins[MAX_INVENTORY_SIZE];
    int instance_ids[MAX_INVENTORY_SIZE];
    int valid_count = 0;

    printf("Loading your inventory...\n");
    for (int i = 0; i < inv.count && i < MAX_INVENTORY_SIZE; i++)
    {
        int instance_id = inv.skin_ids[i];
        if (load_skin_details(instance_id, &skins[valid_count]) == 0)
        {
            instance_ids[valid_count] = instance_id;
            valid_count++;
        }
    }

    if (valid_count == 0)
    {
        print_error("No valid items in inventory");
        wait_for_key();
        return;
    }

    // Build trade offer
    TradeOffer offer;
    memset(&offer, 0, sizeof(TradeOffer));
    offer.from_user_id = g_user_id;
    offer.to_user_id = to_user_id;
    offer.offered_count = 0;
    offer.requested_count = 0;
    offer.offered_cash = 0.0f;
    offer.requested_cash = 0.0f;

    // Load opponent's inventory
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));
    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_INVENTORY;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", to_user_id);
    request.header.msg_length = strlen(request.payload);

    Inventory opp_inv;
    Skin opp_skins[MAX_INVENTORY_SIZE];
    int opp_instance_ids[MAX_INVENTORY_SIZE];
    int opp_valid_count = 0;

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_INVENTORY_DATA)
        {
            memcpy(&opp_inv, response.payload, sizeof(Inventory));
            printf("Loading opponent's inventory...\n");
            for (int i = 0; i < opp_inv.count && i < MAX_INVENTORY_SIZE; i++)
            {
                int instance_id = opp_inv.skin_ids[i];
                if (load_skin_details(instance_id, &opp_skins[opp_valid_count]) == 0)
                {
                    opp_instance_ids[opp_valid_count] = instance_id;
                    opp_valid_count++;
                }
            }
        }
    }

    // Step 1: Select items to REQUEST from opponent
    printf("\n=== Items You're REQUESTING (from %s) ===\n", to_username);
    if (opp_valid_count > 0)
    {
        printf("Opponent's Inventory:\n");
        for (int i = 0; i < opp_valid_count; i++)
        {
            const char *rarity_color = get_rarity_color(opp_skins[i].rarity);
            const char *stattrak = opp_skins[i].is_stattrak ? "StatTrak™ " : "";
            const char *wear = wear_to_string(opp_skins[i].wear);
            // Note: Trade lock status shown for info, but items can still be traded between users
            // Items are only locked when listed on market
            const char *tradable = opp_skins[i].is_tradable ? "" : COLOR_DIM " [Listed on Market]" COLOR_RESET;

            printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d) - $%.2f%s\n",
                   i + 1,
                   rarity_color, rarity_to_string(opp_skins[i].rarity), COLOR_RESET,
                   opp_skins[i].is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
                   opp_skins[i].name, wear, opp_skins[i].pattern_seed, opp_skins[i].current_price,
                   tradable);
        }
    }
    else
    {
        printf("Opponent's inventory is empty.\n");
    }

    printf("\nEnter item numbers to REQUEST (comma-separated, e.g., 1,3,5 or 0 to skip): ");
    fflush(stdout);

    char request_items_input[256];
    if (fgets(request_items_input, sizeof(request_items_input), stdin) == NULL)
        return;

    // Parse requested item numbers
    char *token = strtok(request_items_input, ",\n ");
    while (token != NULL && offer.requested_count < 10)
    {
        int item_num = atoi(token);
        if (item_num > 0 && item_num <= opp_valid_count)
        {
            int idx = item_num - 1;
            // Note: Trade lock check removed - items are only locked when listed on market
            // Trading between users does not require items to be unlocked
            offer.requested_skins[offer.requested_count++] = opp_instance_ids[idx];
        }
        token = strtok(NULL, ",\n ");
    }

    // Step 2: Select items to OFFER from your inventory
    printf("\n=== Items You're OFFERING ===\n");
    printf("Your Inventory:\n");
    for (int i = 0; i < valid_count; i++)
    {
        const char *rarity_color = get_rarity_color(skins[i].rarity);
        const char *stattrak = skins[i].is_stattrak ? "StatTrak™ " : "";
        const char *wear = wear_to_string(skins[i].wear);
        // Note: Items are NOT trade locked when creating offer, only after trade completes

        printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d) - $%.2f\n",
               i + 1,
               rarity_color, rarity_to_string(skins[i].rarity), COLOR_RESET,
               skins[i].is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
               skins[i].name, wear, skins[i].pattern_seed, skins[i].current_price);
    }

    printf("\nEnter item numbers to OFFER (comma-separated, e.g., 1,3,5 or 0 to skip): ");
    fflush(stdout);

    char offer_items_input[256];
    if (fgets(offer_items_input, sizeof(offer_items_input), stdin) == NULL)
        return;

    // Parse offered item numbers
    token = strtok(offer_items_input, ",\n ");
    while (token != NULL && offer.offered_count < 10)
    {
        int item_num = atoi(token);
        if (item_num > 0 && item_num <= valid_count)
        {
            int idx = item_num - 1;
            offer.offered_skins[offer.offered_count++] = instance_ids[idx];
        }
        token = strtok(NULL, ",\n ");
    }

    // Validate: must offer something OR request something
    if (offer.offered_count == 0 && offer.requested_count == 0)
    {
        print_error("You must offer or request at least one item");
        wait_for_key();
        return;
    }

    // Debug: Print trade offer details
    printf("\nTrade Offer Summary:\n");
    printf("  Requesting %d item(s)\n", offer.requested_count);
    printf("  Offering %d item(s)\n", offer.offered_count);
    printf("\nSending trade offer...\n");

    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));
    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_SEND_TRADE_OFFER;
    memcpy(request.payload, &offer, sizeof(TradeOffer));
    request.header.msg_length = sizeof(TradeOffer);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_SEND_TRADE_OFFER)
        {
            TradeOffer created_offer;
            memcpy(&created_offer, response.payload, sizeof(TradeOffer));
            print_success("Trade offer sent successfully!");
            printf("Trade ID: %d\n", created_offer.trade_id);
        }
        else if (response.header.msg_type == MSG_ERROR)
        {
            uint32_t error_code;
            memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));

            if (error_code == ERR_INVALID_TRADE)
                print_error("Invalid trade offer (must offer or request at least one item)");
            else if (error_code == ERR_TRADE_LOCKED)
                print_error("One or more items are trade locked");
            else if (error_code == ERR_PERMISSION_DENIED)
                print_error("You don't own one or more items");
            else if (error_code == ERR_INSUFFICIENT_FUNDS)
                print_error("Insufficient funds");
            else if (error_code == ERR_DATABASE_ERROR)
                print_error("Database error");
            else
            {
                char err_msg[128];
                snprintf(err_msg, sizeof(err_msg), "Failed to send trade offer (error code: %u)", error_code);
                print_error(err_msg);
            }
        }
        else
        {
            print_error("Unexpected response from server");
        }
    }
    else
    {
        print_error("Failed to communicate with server");
    }

    wait_for_key();
}

void show_trading()
{
    clear_screen();
    print_header("TRADING");
    display_balance_info();

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    // Get user trades
    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_TRADES;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0)
    {
        if (receive_message_from_server(&response) == 0)
        {
            if (response.header.msg_type == MSG_TRADES_DATA)
            {
                TradeOffer trades[50];
                int count = 0;
                if (response.header.msg_length > 0)
                {
                    count = response.header.msg_length / sizeof(TradeOffer);
                    if (count > 50)
                        count = 50;
                    memcpy(trades, response.payload, count * sizeof(TradeOffer));
                }

                printf("\nYour Trade Offers:\n\n");

                int pending_count = 0;
                for (int i = 0; i < count; i++)
                {
                    if (trades[i].status == TRADE_PENDING)
                    {
                        pending_count++;
                        printf("%d. Trade #%d: ", pending_count, trades[i].trade_id);
                        if (trades[i].to_user_id == g_user_id)
                        {
                            printf("From User %d -> You\n", trades[i].from_user_id);
                            printf("   Offered: %d skins, $%.2f\n", trades[i].offered_count, trades[i].offered_cash);
                            printf("   Requested: %d skins, $%.2f\n", trades[i].requested_count, trades[i].requested_cash);
                            printf("   [A]ccept [D]ecline\n");
                        }
                        else
                        {
                            printf("You -> User %d\n", trades[i].to_user_id);
                            printf("   Offered: %d skins, $%.2f\n", trades[i].offered_count, trades[i].offered_cash);
                            printf("   Requested: %d skins, $%.2f\n", trades[i].requested_count, trades[i].requested_cash);
                            printf("   [C]ancel\n");
                        }
                        printf("\n");
                    }
                }

                printf("\n");
                print_separator(50);
                printf("Options:\n");
                if (pending_count > 0)
                {
                    printf("  Enter trade number to manage\n");
                }
                printf("  N. Send new trade offer\n");
                printf("  0. Back to main menu\n");
                printf("Select option: ");
                fflush(stdout);

                char input[32];
                if (fgets(input, sizeof(input), stdin) == NULL)
                {
                    wait_for_key();
                    return;
                }

                // Remove newline
                size_t len = strlen(input);
                if (len > 0 && input[len - 1] == '\n')
                    input[len - 1] = '\0';

                if (input[0] == '0' || input[0] == '\0')
                {
                    return; // Back to main menu
                }
                else if (input[0] == 'N' || input[0] == 'n')
                {
                    // Send new trade offer - search user first
                    printf("\nEnter username to send trade offer to: ");
                    fflush(stdout);
                    char username[64];
                    if (fgets(username, sizeof(username), stdin) == NULL)
                    {
                        wait_for_key();
                        return;
                    }

                    // Remove newline
                    len = strlen(username);
                    if (len > 0 && username[len - 1] == '\n')
                        username[len - 1] = '\0';

                    if (strlen(username) == 0)
                    {
                        print_error("Username cannot be empty");
                        sleep(2);
                        return;
                    }

                    User target_user;
                    int result = search_user_by_username(username, &target_user);
                    if (result == 0)
                    {
                        send_trade_offer_ui(target_user.user_id, target_user.username);
                    }
                    else
                    {
                        print_error("User not found");
                        sleep(2);
                    }
                    return;
                }
                else if (pending_count > 0)
                {
                    // Handle trade management
                    int choice = atoi(input);
                    if (choice > 0 && choice <= pending_count)
                    {
                        // Find the selected trade
                        int trade_idx = 0;
                        for (int i = 0; i < count; i++)
                        {
                            if (trades[i].status == TRADE_PENDING)
                            {
                                trade_idx++;
                                if (trade_idx == choice)
                                {
                                    if (trades[i].to_user_id == g_user_id)
                                    {
                                        // Incoming trade - accept or decline
                                        printf("Accept (a) or Decline (d): ");
                                        fflush(stdout);
                                        char action[32];
                                        if (fgets(action, sizeof(action), stdin) != NULL)
                                        {
                                            if (action[0] == 'a' || action[0] == 'A')
                                            {
                                                // Accept trade
                                                memset(&request, 0, sizeof(Message));
                                                memset(&response, 0, sizeof(Message));
                                                request.header.magic = 0xABCD;
                                                request.header.msg_type = MSG_ACCEPT_TRADE;
                                                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, trades[i].trade_id);
                                                request.header.msg_length = strlen(request.payload);

                                                if (send_message_to_server(&request) == 0)
                                                {
                                                    if (receive_message_from_server(&response) == 0)
                                                    {
                                                        if (response.header.msg_type == MSG_TRADE_COMPLETED)
                                                        {
                                                            print_success("Trade accepted successfully!");
                                                        }
                                                        else if (response.header.msg_type == MSG_ERROR)
                                                        {
                                                            uint32_t error_code;
                                                            memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
                                                            if (error_code == ERR_INSUFFICIENT_FUNDS)
                                                                print_error("Insufficient funds for trade");
                                                            else
                                                                print_error("Failed to accept trade");
                                                        }
                                                    }
                                                }
                                            }
                                            else if (action[0] == 'd' || action[0] == 'D')
                                            {
                                                // Decline trade
                                                memset(&request, 0, sizeof(Message));
                                                memset(&response, 0, sizeof(Message));
                                                request.header.magic = 0xABCD;
                                                request.header.msg_type = MSG_DECLINE_TRADE;
                                                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, trades[i].trade_id);
                                                request.header.msg_length = strlen(request.payload);

                                                if (send_message_to_server(&request) == 0)
                                                {
                                                    if (receive_message_from_server(&response) == 0)
                                                    {
                                                        print_success("Trade declined");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // Outgoing trade - cancel
                                        printf("Cancel this trade? (y/n): ");
                                        fflush(stdout);
                                        char action[32];
                                        if (fgets(action, sizeof(action), stdin) != NULL)
                                        {
                                            if (action[0] == 'y' || action[0] == 'Y')
                                            {
                                                memset(&request, 0, sizeof(Message));
                                                memset(&response, 0, sizeof(Message));
                                                request.header.magic = 0xABCD;
                                                request.header.msg_type = MSG_CANCEL_TRADE;
                                                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, trades[i].trade_id);
                                                request.header.msg_length = strlen(request.payload);

                                                if (send_message_to_server(&request) == 0)
                                                {
                                                    if (receive_message_from_server(&response) == 0)
                                                    {
                                                        print_success("Trade cancelled");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                print_error("Failed to load trades");
            }
        }
        else
        {
            print_error("Failed to receive trades");
        }
    }
    else
    {
        print_error("Failed to request trades");
    }

    wait_for_key();
}

// Show quests and achievements
void show_quests_achievements()
{
    clear_screen();
    print_header("QUESTS & ACHIEVEMENTS");
    display_balance_info();

    printf("\n=== DAILY QUESTS ===\n\n");

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_QUESTS;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_QUESTS_DATA && response.header.msg_length > 0)
        {
            int count = response.header.msg_length / sizeof(Quest);
            Quest *quests = (Quest *)response.payload;

            const char *quest_names[] = {
                "First Steps (Complete 3 trades)",
                "Market Explorer (Buy 5 items)",
                "Lucky Gambler (Unbox 5 cases)",
                "Profit Maker (Make $50 profit)",
                "Social Trader (Trade with 10 users)"};

            float quest_rewards[] = {15.0f, 10.0f, 25.0f, 30.0f, 50.0f};

            for (int i = 0; i < count; i++)
            {
                printf("%d. %s\n", i + 1, quest_names[quests[i].quest_type]);
                printf("   Progress: %d/%d\n", quests[i].progress, quests[i].target);
                if (quests[i].is_completed)
                {
                    if (quests[i].is_claimed)
                    {
                        printf("   Status: %s✓ Claimed%s\n", COLOR_GREEN, COLOR_RESET);
                    }
                    else
                    {
                        printf("   Status: %s✓ Completed - Reward: $%.2f%s\n", COLOR_YELLOW, quest_rewards[quests[i].quest_type], COLOR_RESET);
                        printf("   Press %d to claim reward\n", i + 1);
                    }
                }
                else
                {
                    printf("   Status: In Progress\n");
                }
                printf("\n");
            }

            printf("Enter quest number to claim reward, or 0 to go back: ");
            fflush(stdout);
            char choice[32];
            if (fgets(choice, sizeof(choice), stdin) != NULL)
            {
                int quest_num = atoi(choice);
                if (quest_num > 0 && quest_num <= count)
                {
                    int quest_id = quests[quest_num - 1].quest_id;
                    if (quests[quest_num - 1].is_completed && !quests[quest_num - 1].is_claimed)
                    {
                        memset(&request, 0, sizeof(Message));
                        memset(&response, 0, sizeof(Message));
                        request.header.magic = 0xABCD;
                        request.header.msg_type = MSG_CLAIM_QUEST_REWARD;
                        snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, quest_id);
                        request.header.msg_length = strlen(request.payload);

                        if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                        {
                            if (response.header.msg_type == MSG_CLAIM_QUEST_REWARD)
                            {
                                print_success("Quest reward claimed!");
                            }
                            else
                            {
                                print_error("Failed to claim reward");
                            }
                        }
                    }
                }
            }
        }
    }

    printf("\n=== ACHIEVEMENTS ===\n\n");

    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));
    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_ACHIEVEMENTS;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_ACHIEVEMENTS_DATA && response.header.msg_length > 0)
        {
            int count = response.header.msg_length / sizeof(Achievement);
            Achievement *achievements = (Achievement *)response.payload;

            const char *achievement_names[] = {
                "First Trade Completed (+$20)",
                "First Knife Unboxed (+$500)",
                "Total Profit $1,000 (+$100)",
                "100 Successful Trades (+$200)"};

            float achievement_rewards[] = {20.0f, 500.0f, 100.0f, 200.0f};

            for (int i = 0; i < count; i++)
            {
                if (achievements[i].is_unlocked)
                {
                    printf("%s✓%s %s\n", COLOR_GREEN, COLOR_RESET, achievement_names[achievements[i].achievement_type]);
                    if (achievements[i].is_claimed)
                    {
                        printf("   Status: Claimed\n");
                    }
                    else
                    {
                        printf("   Status: %sUnlocked - Reward: $%.2f%s\n", COLOR_YELLOW, achievement_rewards[achievements[i].achievement_type], COLOR_RESET);
                        printf("   Press %d to claim reward\n", i + 1);
                    }
                }
                else
                {
                    printf("%s✗%s %s (Locked)\n", COLOR_RED, COLOR_RESET, achievement_names[achievements[i].achievement_type]);
                }
                printf("\n");
            }

            printf("Enter achievement number to claim reward, or 0 to go back: ");
            fflush(stdout);
            char choice[32];
            if (fgets(choice, sizeof(choice), stdin) != NULL)
            {
                int ach_num = atoi(choice);
                if (ach_num > 0 && ach_num <= count)
                {
                    int achievement_id = achievements[ach_num - 1].achievement_id;
                    if (achievements[ach_num - 1].is_unlocked && !achievements[ach_num - 1].is_claimed)
                    {
                        memset(&request, 0, sizeof(Message));
                        memset(&response, 0, sizeof(Message));
                        request.header.magic = 0xABCD;
                        request.header.msg_type = MSG_CLAIM_ACHIEVEMENT;
                        snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%d", g_user_id, achievement_id);
                        request.header.msg_length = strlen(request.payload);

                        if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                        {
                            if (response.header.msg_type == MSG_CLAIM_ACHIEVEMENT)
                            {
                                print_success("Achievement reward claimed!");
                            }
                            else
                            {
                                print_error("Failed to claim reward");
                            }
                        }
                    }
                }
            }
        }
    }

    wait_for_key();
}

// Show daily rewards
void show_daily_rewards()
{
    clear_screen();
    print_header("DAILY LOGIN REWARDS");
    display_balance_info();

    printf("\n=== CLAIM DAILY REWARD ===\n\n");

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_LOGIN_REWARD;
    snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
    request.header.msg_length = strlen(request.payload);

    if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
    {
        if (response.header.msg_type == MSG_LOGIN_REWARD_DATA)
        {
            float reward_amount = 0.0f;
            int streak_day = 0;
            if (sscanf((char *)response.payload, "%f:%d", &reward_amount, &streak_day) == 2)
            {
                printf("Day %d Reward: $%.2f\n", streak_day, reward_amount);
                print_success("Daily reward claimed!");

                if (streak_day == 7)
                {
                    printf("\n%s🎉 7-Day Streak Complete! Bonus reward!%s\n", COLOR_YELLOW, COLOR_RESET);
                }
            }
        }
        else if (response.header.msg_type == MSG_ERROR)
        {
            print_error("Already claimed today or error occurred");
        }
    }

    printf("\nDaily Rewards:\n");
    printf("Day 1: $5.00\n");
    printf("Day 2: $8.00\n");
    printf("Day 3: $12.00\n");
    printf("Day 4: $15.00\n");
    printf("Day 5: $20.00\n");
    printf("Day 6: $25.00\n");
    printf("Day 7: $50.00 + Bonus\n");
    printf("\nNote: Streak resets if you miss a day\n");

    wait_for_key();
}

// Show chat
void show_chat()
{
    int should_exit = 0;
    while (!should_exit)
    {
        clear_screen();
        print_header("GLOBAL CHAT");
        display_balance_info();

        printf("\n=== CHAT HISTORY ===\n\n");

        // Fetch recent messages from server
        Message request, response;
        memset(&request, 0, sizeof(Message));
        memset(&response, 0, sizeof(Message));

        request.header.magic = 0xABCD;
        request.header.msg_type = MSG_GET_CHAT_HISTORY;
        snprintf(request.payload, MAX_PAYLOAD_SIZE, "50"); // Get last 50 messages
        request.header.msg_length = strlen(request.payload);

        if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
        {
            if (response.header.msg_type == MSG_CHAT_HISTORY_DATA && response.header.msg_length > 0)
            {
                int count = response.header.msg_length / sizeof(ChatMessage);
                ChatMessage *messages = (ChatMessage *)response.payload;

                // Display messages (oldest first)
                for (int i = count - 1; i >= 0; i--)
                {
                    time_t msg_time = messages[i].timestamp;
                    struct tm *time_info = localtime(&msg_time);
                    char time_str[32];
                    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

                    if (messages[i].user_id == 0)
                    {
                        // System message (broadcast)
                        printf("%s[%s] %s%s\n", COLOR_YELLOW, time_str, messages[i].message, COLOR_RESET);
                    }
                    else
                    {
                        printf("%s[%s] %s%s: %s%s\n",
                               COLOR_CYAN, time_str,
                               COLOR_BRIGHT_GREEN, messages[i].username,
                               COLOR_RESET, messages[i].message);
                    }
                }
            }
            else
            {
                printf("No messages yet.\n");
            }
        }
        else
        {
            printf("Failed to load chat history.\n");
        }

        printf("\n=== SEND MESSAGE ===\n");
        printf("Type message (or 'exit' to go back): ");
        fflush(stdout);

        char message[256];
        if (fgets(message, sizeof(message), stdin) != NULL)
        {
            // Remove newline
            size_t len = strlen(message);
            if (len > 0 && message[len - 1] == '\n')
                message[len - 1] = '\0';

            if (strcmp(message, "exit") == 0 || strcmp(message, "Exit") == 0)
            {
                should_exit = 1;
                break;
            }

            if (strlen(message) > 0)
            {
                // Get username
                User user;
                memset(&request, 0, sizeof(Message));
                memset(&response, 0, sizeof(Message));

                request.header.magic = 0xABCD;
                request.header.msg_type = MSG_GET_USER_PROFILE;
                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
                request.header.msg_length = strlen(request.payload);

                char username[MAX_USERNAME_LEN] = "User";
                if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                {
                    if (response.header.msg_type == MSG_USER_PROFILE_DATA)
                    {
                        memcpy(&user, response.payload, sizeof(User));
                        strncpy(username, user.username, MAX_USERNAME_LEN - 1);
                    }
                }

                // Send chat message
                memset(&request, 0, sizeof(Message));
                memset(&response, 0, sizeof(Message));
                request.header.magic = 0xABCD;
                request.header.msg_type = MSG_CHAT_GLOBAL;
                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d:%s:%s", g_user_id, username, message);
                request.header.msg_length = strlen(request.payload);

                if (send_message_to_server(&request) == 0 && receive_message_from_server(&response) == 0)
                {
                    if (response.header.msg_type != MSG_CHAT_GLOBAL)
                    {
                        print_error("Failed to send message");
                        wait_for_key();
                    }
                    // Message sent successfully, loop will refresh and show it
                }
            }
        }
    }
}

// Authentication screen
int authenticate()
{
    clear_screen();
    print_header("CS2 SKIN TRADING SIMULATOR");

    printf("\n");
    print_menu_item("1. Login", 0, 2, 5);
    print_menu_item("2. Register", 0, 2, 6);
    print_menu_item("3. Exit", 0, 2, 7);

    printf("\n");
    print_separator(50);
    printf("Select option (1-3): ");
    fflush(stdout);

    char choice[32];
    if (fgets(choice, sizeof(choice), stdin) == NULL)
        return -1;

    int option = atoi(choice);

    if (option == 1)
    {
        char username[64], password[64];
        if (get_user_input(username, sizeof(username), "Username: ") == 0 &&
            get_password_input(password, sizeof(password), "Password: ") == 0)
        {
            if (handle_login(username, password) == 0)
            {
                print_success("Login successful!");
                sleep(1);
                return 0;
            }
        }
    }
    else if (option == 2)
    {
        char username[64], password[64];
        if (get_user_input(username, sizeof(username), "Username: ") == 0 &&
            get_password_input(password, sizeof(password), "Password: ") == 0)
        {
            if (handle_register(username, password) == 0)
            {
                // Auto login after registration
                if (handle_login(username, password) == 0)
                {
                    print_success("Registration and login successful!");
                    sleep(1);
                    return 0;
                }
            }
        }
    }
    else if (option == 3)
    {
        // Exit: disconnect and exit program
        disconnect_from_server();
        return -2; // Special return code for exit
    }

    return -1;
}

// Main loop
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    printf("=== CS2 Skin Trading Client ===\n");
    printf("Connecting to %s:%d...\n", server_ip, port);

    // Connect to server
    if (connect_to_server(server_ip, port) != 0)
    {
        print_error("Failed to connect to server");
        return 1;
    }

    print_success("Connected to server!");
    sleep(1);

    // Outer loop: authenticate and main menu
    int should_exit = 0;
    while (!should_exit && is_connected())
    {
        // Authenticate
        int auth_result = authenticate();
        if (auth_result == -2)
        {
            // Exit requested
            should_exit = 1;
            break;
        }
        while (auth_result != 0 && !should_exit)
        {
            // Retry or exit
            printf("\nPress Enter to retry or Ctrl+C to exit...");
            getchar();
            auth_result = authenticate();
            if (auth_result == -2)
            {
                should_exit = 1;
                break;
            }
        }

        if (should_exit)
            break;

        // Main menu loop
        int running = 1;
        while (running && is_connected() && !should_exit)
        {
            show_main_menu();

            char choice[32];
            if (fgets(choice, sizeof(choice), stdin) == NULL)
            {
                should_exit = 1;
                break;
            }

            int option = atoi(choice);

            switch (option)
            {
            case 1:
                show_inventory();
                break;
            case 2:
                show_market();
                break;
            case 3:
                show_trading();
                break;
            case 4:
                show_unbox();
                break;
            case 5:
                show_profile();
                break;
            case 6:
                show_quests_achievements();
                break;
            case 7:
                show_daily_rewards();
                break;
            case 8:
                show_chat();
                break;
            case 9:
                show_leaderboards();
                break;
            case 10:
                show_trade_analytics();
                break;
            case 11:
                show_trading_challenges();
                break;
            case 12:
            {
                // Logout: send logout message and return to authenticate
                Message request, response;
                memset(&request, 0, sizeof(Message));
                memset(&response, 0, sizeof(Message));

                request.header.magic = 0xABCD;
                request.header.msg_type = MSG_LOGOUT;
                snprintf(request.payload, MAX_PAYLOAD_SIZE, "%d", g_user_id);
                request.header.msg_length = strlen(request.payload);

                send_message_to_server(&request);
                receive_message_from_server(&response);

                // Reset user ID
                g_user_id = -1;

                // Break out of main menu loop to return to authenticate
                running = 0;
                break;
            }
            case 13:
                // Exit: set flag to exit completely
                should_exit = 1;
                running = 0;
                break;
            default:
                print_error("Invalid option");
                sleep(1);
                break;
            }
        }
    }

    // Cleanup
    disconnect_from_server();
    clear_screen();
    printf("Goodbye!\n");

    return 0;
}
