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
    clear_screen();
    print_header("INVENTORY");

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
        }
        else
        {
            printf("\n");
            for (int i = 0; i < inv.count && i < MAX_INVENTORY_SIZE; i++)
            {
                int instance_id = inv.skin_ids[i];

                // Load full skin details by querying database through a helper
                // We'll create a simple query to get skin instance + definition
                // For now, we'll need to add a message type or use existing ones
                // Let's use a workaround: query each skin individually

                // Create a helper function to load skin details
                // This is a simplified version - in production would batch load
                Skin skin;
                if (load_skin_details(instance_id, &skin) == 0)
                {
                    const char *rarity_color = get_rarity_color(skin.rarity);
                    const char *stattrak = skin.is_stattrak ? "StatTrak™ " : "";
                    const char *wear = wear_to_string(skin.wear);
                    const char *tradable = skin.is_tradable ? "" : " [Trade Locked]";

                    printf("%d. %s[%s]%s %s%s%s%s (%s, Pattern #%d) - $%.2f%s\n",
                           i + 1,
                           rarity_color, rarity_to_string(skin.rarity), COLOR_RESET,
                           skin.is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
                           skin.name, wear, skin.pattern_seed, skin.current_price,
                           tradable);
                }
                else
                {
                    printf("%d. Skin Instance ID: %d (Failed to load details)\n", i + 1, instance_id);
                }
            }
        }
    }
    else
    {
        print_error("Failed to load inventory");
    }

    wait_for_key();
}

void show_market()
{
    clear_screen();
    print_header("MARKET");

    Message request, response;
    memset(&request, 0, sizeof(Message));
    memset(&response, 0, sizeof(Message));

    request.header.magic = 0xABCD;
    request.header.msg_type = MSG_GET_MARKET_LISTINGS;
    request.header.msg_length = 0;

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
        }
        else
        {
            for (int i = 0; i < count; i++)
            {
                printf("%d. Listing ID: %d - Price: $%.2f\n",
                       i + 1, listings[i].listing_id, listings[i].price);
            }
        }
    }
    else
    {
        print_error("Failed to load market listings");
    }

    wait_for_key();
}

void show_unbox()
{
    clear_screen();
    print_header("UNBOX CASES");

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
    clear_screen();
    print_header("PROFILE");

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

                printf("\n");
                print_box(10, 5, 60, 15, "User Profile");
                move_cursor(7, 12);
                printf("Username: %s%s%s\n", STYLE_BOLD, user.username, COLOR_RESET);
                move_cursor(8, 12);
                printf("Balance: %s$%.2f%s\n", COLOR_GREEN, user.balance, COLOR_RESET);
                move_cursor(9, 12);
                printf("User ID: %d\n", user.user_id);
            }
        }
    }

    wait_for_key();
}

void show_trading()
{
    clear_screen();
    print_header("TRADING");

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

                if (pending_count == 0)
                {
                    print_info("No pending trade offers");
                }
                else
                {
                    printf("\nSelect trade number to manage (0 to cancel): ");
                    fflush(stdout);

                    char input[32];
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
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
        return -1;
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
        while (authenticate() != 0)
        {
            // Retry or exit
            printf("\nPress Enter to retry or Ctrl+C to exit...");
            getchar();
        }

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
