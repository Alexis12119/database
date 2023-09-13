#include <cstring>
#include <iomanip>
#include <iostream>

#include "include/sqlite3.h"

// Constants for menu choices
const int MENU_ADD_BOOK = 1;
const int MENU_VIEW_BOOKS = 2;
const int MENU_DELETE_BOOK = 3;
const int MENU_SEARCH_BOOK = 4;
const int MENU_QUIT = 5;

void displayMenu();
void addBook(sqlite3* db);
void viewBooks(sqlite3* db);
void searchBooks(sqlite3* db);
void deleteBook(sqlite3* db);
void handleSqliteError(sqlite3* db, const char* operation);
bool checkIfExists(sqlite3* db, int bookId);

// Callback function for querying the database
static int callback(void* data, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            std::cout << " | ";
        }
        std::cout << std::left << std::setw(i == 1 ? 24 : (i == 2 ? 16 : 8))
                  << (argv[i] ? argv[i] : "NULL");
    }
    std::cout << "\n";
    return 0;
}

// Class to manage SQLite database connection with RAII
class DatabaseConnection {
   private:
    sqlite3* db;

   public:
    DatabaseConnection() : db(nullptr) {
        int rc = sqlite3_open("books.db", &db);
        if (rc) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
            throw std::runtime_error("Database connection error");
        }
    }

    ~DatabaseConnection() {
        if (db) {
            sqlite3_close(db);
        }
    }

    sqlite3* get() const {
        return db;
    }
};

int main() {
    try {
        DatabaseConnection dbConnection;
        sqlite3* db = dbConnection.get();

        // Create a table to store the book data if it doesn't exist
        const char* createTableSQL
            = "CREATE TABLE IF NOT EXISTS books (id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "title TEXT UNIQUE, author TEXT);";
        int rc = sqlite3_exec(db, createTableSQL, callback, 0, nullptr);

        if (rc != SQLITE_OK) {
            handleSqliteError(db, "SQL table creation");
            return 1;
        }

        while (true) {
            displayMenu();

            int choice;
            std::cin >> choice;

            switch (choice) {
            case MENU_ADD_BOOK:
                addBook(db);
                break;
            case MENU_VIEW_BOOKS:
                viewBooks(db);
                break;
            case MENU_DELETE_BOOK:
                deleteBook(db);
                break;
            case MENU_SEARCH_BOOK:
                searchBooks(db);
                break;
            case MENU_QUIT:
                // Close the database and exit
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }
}

// Function to display the menu
void displayMenu() {
    std::cout << "\n*** Book Management System ***\n";
    std::cout << "1. Add a book\n";
    std::cout << "2. View all books\n";
    std::cout << "3. Delete a book\n";
    std::cout << "4. Search a book\n";
    std::cout << "5. Quit\n";
    std::cout << "Enter your choice: ";
}

// Function to add a book to the database
void addBook(sqlite3* db) {
    while (true) {
        std::string title, author;
        std::cout << "Enter the title of the book: ";
        std::cin.ignore();
        std::getline(std::cin, title);

        // Check if a book with the same title already exists
        const char* checkTitleSQL = "SELECT id FROM books WHERE title = ?;";
        sqlite3_stmt* checkStmt;
        int rc = sqlite3_prepare_v2(db, checkTitleSQL, -1, &checkStmt, nullptr);
        if (rc != SQLITE_OK) {
            handleSqliteError(db, "prepare statement");
            return;
        }

        sqlite3_bind_text(checkStmt, 1, title.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(checkStmt);

        if (rc == SQLITE_ROW) {
            std::cout << "A book with the same title already exists in the database.\n";
            sqlite3_finalize(checkStmt);
            // Ask if the user wants to add another book
            char tryAgain;
            std::cout << "\nDo you want to try again? (y/n): ";
            std::cin >> tryAgain;
            if (tryAgain != 'y' && tryAgain != 'Y') {
                break;  // Return to the main menu
            }
        }

        // If no duplicate title, continue with adding the book
        std::cout << "Enter the author of the book: ";
        std::getline(std::cin, author);

        // Use parameterized query to insert the book
        const char* insertSQL = "INSERT INTO books (title, author) VALUES (?, ?);";
        sqlite3_stmt* stmt;

        rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            handleSqliteError(db, "prepare statement");
            sqlite3_finalize(checkStmt);
        }

        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, author.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            handleSqliteError(db, "execute statement");
        } else {
            std::cout << "Book added successfully.\n";
        }

        sqlite3_finalize(stmt);
        sqlite3_finalize(checkStmt);
    }
}

// Function to view books with sorting
void viewBooks(sqlite3* db) {
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

        std::string sortOrder
            = (sortOrderChoice == 2) ? "DESC" : "ASC";  // Default to ascending for other choices

        // View all books in the database with the selected sorting criteria and order
        std::string selectSQL = "SELECT * FROM books ORDER BY " + orderBy + " " + sortOrder + ";";

        std::cout << std::left << std::setw(8) << "ID";
        std::cout << " | ";
        std::cout << std::left << std::setw(24) << "Title";
        std::cout << " | ";
        std::cout << std::left << std::setw(16) << "Author"
                  << "\n";

        std::cout << std::setfill('=') << std::setw(8) << ""
                  << "=";
        std::cout << std::setw(26) << ""
                  << "=";
        std::cout << std::setw(18) << ""
                  << "\n";
        std::cout << std::setfill(' ');

        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, selectSQL.c_str(), callback, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << zErrMsg << "\n";
            sqlite3_free(zErrMsg);
        }
    }
}

// Function to search for books by title or author
void searchBooks(sqlite3* db) {
    while (true) {
        std::string searchTerm;
        std::cout << "Enter search term (title or author): ";
        std::cin.ignore();
        std::getline(std::cin, searchTerm);

        // Construct a SQL query to search for books
        std::string searchSQL = "SELECT * FROM books WHERE title LIKE '%" + searchTerm
                                + "%' OR author LIKE '%" + searchTerm + "%';";

        std::cout << "Search Results:\n";
        std::cout << std::left << std::setw(8) << "ID";
        std::cout << " | ";
        std::cout << std::left << std::setw(24) << "Title";
        std::cout << " | ";
        std::cout << std::left << std::setw(16) << "Author"
                  << "\n";

        std::cout << std::setfill('=') << std::setw(8) << ""
                  << "=";
        std::cout << std::setw(26) << ""
                  << "=";
        std::cout << std::setw(18) << ""
                  << "\n";
        std::cout << std::setfill(' ');

        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, searchSQL.c_str(), callback, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << zErrMsg << "\n";
            sqlite3_free(zErrMsg);
        }
        // Ask if the user wants to search again
        char tryAgain;
        std::cout << "\nDo you want to search again? (y/n): ";
        std::cin >> tryAgain;
        if (tryAgain != 'y' && tryAgain != 'Y') {
            break;  // Return to the main menu
        }
    }
}

// Function to delete a book from the database
void deleteBook(sqlite3* db) {
    int bookId;
    std::cout << "Enter the ID of the book you want to delete: ";
    std::cin >> bookId;

    // Check if the book with the specified ID exists
    if (!checkIfExists(db, bookId)) {
        std::cout << "Book with ID " << bookId << " does not exist in the database.\n";
        return;
    }

    char confirm;
    std::cout << "Are you sure you want to delete this book? (y/n): ";
    std::cin >> confirm;

    if (confirm == 'y' || confirm == 'Y') {
        // Use parameterized query to delete the book
        const char* deleteSQL = "DELETE FROM books WHERE id = ?;";
        sqlite3_stmt* deleteStmt;

        int rc = sqlite3_prepare_v2(db, deleteSQL, -1, &deleteStmt, nullptr);
        if (rc != SQLITE_OK) {
            handleSqliteError(db, "prepare statement");
            return;
        }

        sqlite3_bind_int(deleteStmt, 1, bookId);

        rc = sqlite3_step(deleteStmt);
        if (rc != SQLITE_DONE) {
            handleSqliteError(db, "execute statement");
        } else {
            std::cout << "Book deleted successfully.\n";
        }

        sqlite3_finalize(deleteStmt);
    } else {
        std::cout << "Deletion canceled.\n";
    }
}

bool checkIfExists(sqlite3* db, int bookId) {
    const char* checkSQL = "SELECT 1 FROM books WHERE id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, checkSQL, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        handleSqliteError(db, "prepare statement");
        return false;
    }

    sqlite3_bind_int(stmt, 1, bookId);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        // Book with the specified ID exists
        sqlite3_finalize(stmt);
        return true;
    } else {
        // Book with the specified ID does not exist
        sqlite3_finalize(stmt);
        return false;
    }
}

void handleSqliteError(sqlite3* db, const char* operation) {
    std::cerr << "SQLite error during " << operation << ": " << sqlite3_errmsg(db) << "\n";
}
