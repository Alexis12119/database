#include <cstring>  // Include this for strlen
#include <iomanip>
#include <iostream>

#include "include/sqlite3.h"

// Callback function for querying the database
static int callback(void* data, int argc, char** argv, char** azColName) {
    static bool headersPrinted = false; // Track whether headers are already printed

    if (!headersPrinted) {
        std::cout << std::left << std::setw(8) << "ID";
        std::cout << " | ";
        std::cout << std::left << std::setw(24) << "Title";
        std::cout << " | ";
        std::cout << std::left << std::setw(16) << "Author" << "\n";

        std::cout << std::setfill('=') << std::setw(8) << ""
                  << "=";
        std::cout << std::setw(26) << ""
                  << "=";
        std::cout << std::setw(18) << "" << "\n";
        std::cout << std::setfill(' ');

        headersPrinted = true;
    }

    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            std::cout << " | ";
        }
        std::cout << std::left << std::setw(i == 1 ? 24 : (i == 2 ? 16 : 8)) << (argv[i] ? argv[i] : "NULL");
    }
    std::cout << "\n";
    return 0;
}

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    // Open or create the database
    rc = sqlite3_open("books.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return 0;
    }

    // Create a table to store the book data if it doesn't exist
    const char *createTableSQL
        = "CREATE TABLE IF NOT EXISTS books (id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "title TEXT, author TEXT);";
    rc = sqlite3_exec(db, createTableSQL, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
    }

    while (true) {
        std::cout << "\n*** Book Management System ***\n";
        std::cout << "1. Add a book\n";
        std::cout << "2. View all books\n";
        std::cout << "3. Delete a book\n";
        std::cout << "4. Quit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1: {
            std::string title, author;
            std::cout << "Enter the title of the book: ";
            std::cin.ignore();
            std::getline(std::cin, title);
            std::cout << "Enter the author of the book: ";
            std::getline(std::cin, author);

            // Insert the book into the database
            std::string insertSQL
                = "INSERT INTO books (title, author) VALUES ('" + title + "', '" + author + "');";
            rc = sqlite3_exec(db, insertSQL.c_str(), callback, 0, &zErrMsg);

            if (rc != SQLITE_OK) {
                std::cerr << "SQL error: " << zErrMsg << "\n";
                sqlite3_free(zErrMsg);
            } else {
                std::cout << "Book added successfully.\n";
            }
            break;
        }
        case 2: {
            // Prompt the user for sorting criteria
            std::cout << "Select sorting criterion:\n";
            std::cout << "1. Sort by Title\n";
            std::cout << "2. Sort by Author\n";
            std::cout << "Enter your choice: ";

            int sortChoice;
            std::cin >> sortChoice;

            std::string orderBy;

            switch (sortChoice) {
            case 1:
                orderBy = "title";
                break;
            case 2:
                orderBy = "author";
                break;
            default:
                std::cout << "Invalid choice. Books will not be sorted.\n";
                break;
            }

            if (!orderBy.empty()) {
                // Prompt the user for sorting order
                std::cout << "Select sorting order:\n";
                std::cout << "1. Ascending\n";
                std::cout << "2. Descending\n";
                std::cout << "Enter your choice: ";

                int sortOrderChoice;
                std::cin >> sortOrderChoice;

                std::string sortOrder = (sortOrderChoice == 2)
                                            ? "DESC"
                                            : "ASC";  // Default to ascending for other choices

                // View all books in the database with the selected sorting criteria and order
                std::string selectSQL
                    = "SELECT * FROM books ORDER BY " + orderBy + " " + sortOrder + ";";
                rc = sqlite3_exec(db, selectSQL.c_str(), callback, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    std::cerr << "SQL error: " << zErrMsg << "\n";
                    sqlite3_free(zErrMsg);
                }
            }
            break;
        }

        case 3: {
            int bookId;
            std::cout << "Enter the ID of the book you want to delete: ";
            std::cin >> bookId;

            // Prepare a statement to check if the book with the specified ID exists
            sqlite3_stmt *stmt;
            const char *checkSQL = "SELECT 1 FROM books WHERE id = ?;";

            rc = sqlite3_prepare_v2(db, checkSQL, -1, &stmt, NULL);

            if (rc != SQLITE_OK) {
                std::cerr << "SQL error: " << sqlite3_errmsg(db) << "\n";
            } else {
                // Bind the bookId parameter
                sqlite3_bind_int(stmt, 1, bookId);

                // Execute the query
                rc = sqlite3_step(stmt);

                if (rc == SQLITE_ROW) {
                    // Book with the specified ID exists
                    // Confirmation prompt
                    char confirm;
                    std::cout << "Are you sure you want to delete this book? (y/n): ";
                    std::cin >> confirm;

                    if (confirm == 'y' || confirm == 'Y') {
                        // Delete the book from the database
                        std::string deleteSQL
                            = "DELETE FROM books WHERE id = " + std::to_string(bookId) + ";";
                        rc = sqlite3_exec(db, deleteSQL.c_str(), callback, 0, &zErrMsg);

                        if (rc != SQLITE_OK) {
                            std::cerr << "SQL error: " << zErrMsg << "\n";
                            sqlite3_free(zErrMsg);
                        } else {
                            std::cout << "Book deleted successfully.\n";
                        }
                    } else {
                        std::cout << "Deletion canceled.\n";
                    }
                } else {
                    std::cout << "Book with ID " << bookId << " does not exist in the database.\n";
                }

                // Finalize the statement
                sqlite3_finalize(stmt);
            }
            break;
        }
        case 4:
            // Close the database and exit
            sqlite3_close(db);
            return 0;

        default:
            std::cout << "Invalid choice. Please try again.\n";
            break;
        }
    }
}
