#include <iostream>
#include "include/sqlite3.h"

// Callback function for querying the database
static int callback(void *data, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
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
        return(0);
    }

    // Create a table to store the book data if it doesn't exist
    const char *createTableSQL = "CREATE TABLE IF NOT EXISTS books (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, author TEXT);";
    rc = sqlite3_exec(db, createTableSQL, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
    }

    while (true) {
        std::cout << "\nOptions:\n";
        std::cout << "1. Add a book\n";
        std::cout << "2. View all books\n";
        std::cout << "3. Delete a book\n";
        std::cout << "4. Quit\n";

        int choice;
        std::cout << "Enter your choice: ";
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
                std::string insertSQL = "INSERT INTO books (title, author) VALUES ('" + title + "', '" + author + "');";
                rc = sqlite3_exec(db, insertSQL.c_str(), callback, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    std::cerr << "SQL error: " << zErrMsg << "\n";
                    sqlite3_free(zErrMsg);
                }

                break;
            }

            case 2: {
                // View all books in the database
                const char *selectSQL = "SELECT * FROM books;";
                rc = sqlite3_exec(db, selectSQL, callback, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    std::cerr << "SQL error: " << zErrMsg << "\n";
                    sqlite3_free(zErrMsg);
                }

                break;
            }

            case 3: {
                int bookId;
                std::cout << "Enter the ID of the book you want to delete: ";
                std::cin >> bookId;

                // Delete the book from the database
                std::string deleteSQL = "DELETE FROM books WHERE id = " + std::to_string(bookId) + ";";
                rc = sqlite3_exec(db, deleteSQL.c_str(), callback, 0, &zErrMsg);

                if (rc != SQLITE_OK) {
                    std::cerr << "SQL error: " << zErrMsg << "\n";
                    sqlite3_free(zErrMsg);
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

    // Close the database
    sqlite3_close(db);
    return 0;
}
