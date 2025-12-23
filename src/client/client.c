// client.c - Main Client Implementation (Phase 9)

#include "../include/ui.h"
#include "../include/network_client.h"
#include "../include/protocol.h"
#include "../include/types.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static int g_user_id = -1;
static char g_session_token[37] = {0};

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
    printf("%sBalance: %s$%.2f%s | Inventory Value: %s$%.2f%s | Total: %s$%.2f%s%s\n",
           COLOR_CYAN, COLOR_GREEN, balance, COLOR_RESET,
           COLOR_YELLOW, inv_value, COLOR_RESET,
           COLOR_BRIGHT_GREEN, total, COLOR_RESET, COLOR_RESET);
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
        strncpy(g_session_token, (char *)response.payload, sizeof(g_session_token) - 1);
        g_session_token[sizeof(g_session_token) - 1] = '\0';
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
    print_menu_item("6. Logout", 0, 2, 10);
    print_menu_item("7. Exit", 0, 2, 11);

    printf("\n");
    print_separator(50);
    printf("Select option (1-7): ");
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
                        }
                        else if (response.header.msg_type == MSG_ERROR)
                        {
                            uint32_t error_code;
                            memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));
                            if (error_code == ERR_PERMISSION_DENIED)
                                print_error("You don't own this item");
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
                    const char *owner_note = (listings[i].seller_id == g_user_id) ? " [Your Listing]" : "";

                    printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d)\n",
                           i + 1,
                           rarity_color, rarity_to_string(skins[i].rarity), COLOR_RESET,
                           skins[i].is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
                           skins[i].name, wear, skins[i].pattern_seed);
                    printf("   Price: %s$%.2f%s%s\n",
                           COLOR_BRIGHT_GREEN, listings[i].price, COLOR_RESET, owner_note);
                    printf("   Listing ID: %d\n\n", listings[i].listing_id);
                }

                printf("\n");
                print_separator(50);
                printf("Options:\n");
                printf("  Enter listing number to buy\n");
                printf("  R<number> to remove your listing (e.g., R1)\n");
                printf("  S. Search by name\n");
                printf("  C. Clear search\n");
                printf("  0. Back to main menu\n");
                printf("Select option: ");
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

                        // Confirm purchase
                        printf("\nPurchase %s%s%s%s for %s$%.2f%s? (y/n): ",
                               skins[listing_idx].is_stattrak ? COLOR_BRIGHT_GREEN : "",
                               skins[listing_idx].is_stattrak ? "StatTrak™ " : "",
                               COLOR_RESET,
                               skins[listing_idx].name,
                               COLOR_BRIGHT_GREEN, listings[listing_idx].price, COLOR_RESET);
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
                                print_success("Item purchased successfully!");
                                printf("Added to your inventory.\n");
                            }
                            else if (response.header.msg_type == MSG_ERROR)
                            {
                                uint32_t error_code;
                                memcpy(&error_code, response.payload + sizeof(uint16_t), sizeof(uint32_t));

                                if (error_code == ERR_INSUFFICIENT_FUNDS)
                                    print_error("Insufficient funds");
                                else if (error_code == ERR_ITEM_NOT_FOUND)
                                    print_error("Listing not found or already sold");
                                else if (error_code == ERR_PERMISSION_DENIED)
                                    print_error("Cannot buy your own listing");
                                else
                                    print_error("Failed to purchase item");
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
        int count = response.header.msg_length / sizeof(Case);
        if (count > 50)
            count = 50;

        memcpy(cases, response.payload, count * sizeof(Case));

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
            if (choice > 0 && choice <= count)
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
                                memcpy(&unboxed, response.payload, sizeof(Skin));

                                // Simple animation: show opening effect
                                clear_screen();
                                printf("\n\n");
                                for (int i = 0; i < 3; i++)
                                {
                                    printf("  Opening case");
                                    for (int j = 0; j < 3; j++)
                                    {
                                        printf(".");
                                        fflush(stdout);
                                        usleep(200000); // 200ms
                                    }
                                    printf("\r");
                                    printf("                \r"); // Clear line
                                    fflush(stdout);
                                    usleep(200000);
                                }

                                // Display unboxed skin
                                clear_screen();
                                printf("\n\n");
                                print_box(20, 3, 60, 12, "UNBOXED!");
                                move_cursor(5, 22);
                                print_skin(&unboxed, 22, 5);
                                move_cursor(11, 22);
                                print_success("Case opened successfully!");
                                printf("\n\n");
                            }
                            else
                            {
                                print_error("Invalid response size from server");
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
                        }
                        else
                        {
                            print_error("Failed to unbox case");
                        }
                    }
                    else
                    {
                        print_error("Failed to receive unbox response");
                    }
                }
                else
                {
                    print_error("Failed to send unbox request");
                }
            }
        }
    }
    else
    {
        print_error("Failed to load cases");
    }

    wait_for_key();
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

    // Step 1: Select items to offer
    printf("\n=== Items You're Offering ===\n");
    printf("Your Inventory:\n");
    for (int i = 0; i < valid_count; i++)
    {
        const char *rarity_color = get_rarity_color(skins[i].rarity);
        const char *stattrak = skins[i].is_stattrak ? "StatTrak™ " : "";
        const char *wear = wear_to_string(skins[i].wear);
        const char *tradable = skins[i].is_tradable ? "" : " [Trade Locked]";

        printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d) - $%.2f%s\n",
               i + 1,
               rarity_color, rarity_to_string(skins[i].rarity), COLOR_RESET,
               skins[i].is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
               skins[i].name, wear, skins[i].pattern_seed, skins[i].current_price,
               tradable);
    }

    printf("\nEnter item numbers to offer (comma-separated, e.g., 1,3,5 or 0 to skip): ");
    fflush(stdout);

    char items_input[256];
    if (fgets(items_input, sizeof(items_input), stdin) == NULL)
        return;

    // Parse item numbers
    char *token = strtok(items_input, ",\n ");
    while (token != NULL && offer.offered_count < 10)
    {
        int item_num = atoi(token);
        if (item_num > 0 && item_num <= valid_count)
        {
            int idx = item_num - 1;
            if (skins[idx].is_tradable)
            {
                offer.offered_skins[offer.offered_count++] = instance_ids[idx];
            }
            else
            {
                printf("Item %d is trade locked, skipping...\n", item_num);
            }
        }
        token = strtok(NULL, ",\n ");
    }

    // Step 2: Enter cash to offer
    printf("\nEnter cash amount to offer (0 to skip): $");
    fflush(stdout);
    char cash_input[32];
    if (fgets(cash_input, sizeof(cash_input), stdin) != NULL)
    {
        offer.offered_cash = atof(cash_input);
        if (offer.offered_cash < 0)
            offer.offered_cash = 0.0f;
    }

    // Step 3: Enter cash to request
    printf("Enter cash amount to request (0 to skip): $");
    fflush(stdout);
    char request_cash_input[32];
    if (fgets(request_cash_input, sizeof(request_cash_input), stdin) != NULL)
    {
        offer.requested_cash = atof(request_cash_input);
        if (offer.requested_cash < 0)
            offer.requested_cash = 0.0f;
    }

    // Validate: must offer something
    if (offer.offered_count == 0 && offer.offered_cash == 0.0f)
    {
        print_error("You must offer at least one item or cash");
        wait_for_key();
        return;
    }

    // Step 4: Send trade offer
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
                print_error("Invalid trade offer");
            else if (error_code == ERR_TRADE_LOCKED)
                print_error("One or more items are trade locked");
            else if (error_code == ERR_PERMISSION_DENIED)
                print_error("You don't own one or more items");
            else if (error_code == ERR_INSUFFICIENT_FUNDS)
                print_error("Insufficient funds");
            else
                print_error("Failed to send trade offer");
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
            case 7:
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
