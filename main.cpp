#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

#include "include/sqlite3.h"

// Constants for menu choices
const int MENU_ADD_BOOK = 1;
const int MENU_VIEW_BOOKS = 2;
const int MENU_DELETE_BOOK = 3;
const int MENU_SEARCH_BOOK = 4;
const int MENU_UPDATE_BOOK = 5;
const int MENU_QUIT = 6;

void displayMenu();
void addBook(sqlite3* db);
void viewBooks(sqlite3* db);
void searchBooks(sqlite3* db);
void deleteBook(sqlite3* db);
void updateBook(sqlite3* db);
void handleSqliteError(sqlite3* db, const char* operation);
bool checkIfExists(sqlite3* db, int bookId);
int getValidIntegerInput();

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

// Enum for log levels
enum LogLevel { INFO, WARNING, ERROR, DEBUG };

// Global log file
std::ofstream logFile("book_management.log");

// Function to write log messages
void writeToLog(LogLevel level, const std::string& message) {
    std::string levelStr;
    switch (level) {
    case INFO:
        levelStr = "INFO";
        break;
    case WARNING:
        levelStr = "WARNING";
        break;
    case ERROR:
        levelStr = "ERROR";
        break;
    case DEBUG:
        levelStr = "DEBUG";
        break;
    }

    std::time_t now = std::time(nullptr);
    struct tm localTime;
    localtime_s(&localTime, &now);  // Windows-specific; use localtime() on other platforms

    logFile << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] ";
    logFile << "[" << levelStr << "] " << message << std::endl;
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

            int choice = getValidIntegerInput();

            switch (choice) {
            case MENU_ADD_BOOK:
                writeToLog(INFO, "User selected to add a book.");
                addBook(db);
                break;
            case MENU_VIEW_BOOKS:
                writeToLog(INFO, "User selected to view books.");
                viewBooks(db);
                break;
            case MENU_DELETE_BOOK:
                writeToLog(INFO, "User selected to delete a book.");
                deleteBook(db);
                break;
            case MENU_SEARCH_BOOK:
                writeToLog(INFO, "User selected to search for a book.");
                searchBooks(db);
                break;
            case MENU_UPDATE_BOOK:
                writeToLog(INFO, "User selected to update a book.");
                updateBook(db);
                break;
            case MENU_QUIT:
                writeToLog(INFO, "User selected to quit.");
                // Close the log file
                logFile.close();
                // Close the database and exit
                return 0;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
            }
        }
        // Close the log file
        logFile.close();

        // Close the database and exit
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        // Close the log file
        logFile.close();
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
    std::cout << "5. Update a book\n";
    std::cout << "6. Quit\n";
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
            return;
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

    int sortChoice = getValidIntegerInput();

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

        int sortOrderChoice = getValidIntegerInput();

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

// Function to search for books by title or author with parameterized query
void searchBooks(sqlite3* db) {
    while (true) {
        std::string searchTerm;
        std::cout << "Enter search term (title or author): ";
        std::cin.ignore();
        std::getline(std::cin, searchTerm);

        // Construct a SQL query to search for books with parameterized query
        const char* searchSQL = "SELECT * FROM books WHERE title LIKE ? OR author LIKE ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, searchSQL, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            handleSqliteError(db, "prepare statement");
            return;
        }

        // Bind the search term to the parameter
        sqlite3_bind_text(stmt, 1, ("%" + searchTerm + "%").c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ("%" + searchTerm + "%").c_str(), -1, SQLITE_STATIC);

        // Display header
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

        // Step through the prepared statement and print results row by row
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::cout << std::left << std::setw(8) << sqlite3_column_int(stmt, 0);
            std::cout << " | ";
            std::cout << std::left << std::setw(24)
                      << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::cout << " | ";
            std::cout << std::left << std::setw(16)
                      << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) << "\n";
        }

        if (rc != SQLITE_DONE) {
            handleSqliteError(db, "execute statement");
        }

        sqlite3_finalize(stmt);

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
    std::cout << "Enter the ID of the book you want to delete: ";
    int bookId = getValidIntegerInput();

    // Retrieve the title and author information based on the book ID
    std::string title, author;
    const char* selectSQL = "SELECT title, author FROM books WHERE id = ?;";
    sqlite3_stmt* selectStmt;

    int rc = sqlite3_prepare_v2(db, selectSQL, -1, &selectStmt, nullptr);
    if (rc != SQLITE_OK) {
        handleSqliteError(db, "prepare statement");
        return;
    }

    sqlite3_bind_int(selectStmt, 1, bookId);

    rc = sqlite3_step(selectStmt);
    if (rc == SQLITE_ROW) {
        title = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 0));
        author = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 1));
    }

    sqlite3_finalize(selectStmt);

    // Ask for confirmation before deleting the book
    std::cout << "You are about to delete the following book:\n";
    std::cout << "Title: " << title << "\n";
    std::cout << "Author: " << author << "\n";

    char confirm;
    std::cout << "Are you sure you want to delete this book? (y/n): ";
    std::cin >> confirm;

    if (confirm == 'y' || confirm == 'Y') {
        // Use parameterized query to delete the book
        const char* deleteSQL = "DELETE FROM books WHERE id = ?;";
        sqlite3_stmt* deleteStmt;

        rc = sqlite3_prepare_v2(db, deleteSQL, -1, &deleteStmt, nullptr);
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

// Function to update a book in the database
void updateBook(sqlite3* db) {
    std::cout << "Enter the ID of the book you want to update: ";
    int bookId = getValidIntegerInput();

    // Check if the book with the specified ID exists
    if (!checkIfExists(db, bookId)) {
        std::cout << "Book with ID " << bookId << " does not exist in the database.\n";
        return;
    }

    // Retrieve the current title and author information based on the book ID
    std::string currentTitle, currentAuthor;
    const char* selectSQL = "SELECT title, author FROM books WHERE id = ?;";
    sqlite3_stmt* selectStmt;

    int rc = sqlite3_prepare_v2(db, selectSQL, -1, &selectStmt, nullptr);
    if (rc != SQLITE_OK) {
        handleSqliteError(db, "prepare statement");
        return;
    }

    sqlite3_bind_int(selectStmt, 1, bookId);

    rc = sqlite3_step(selectStmt);
    if (rc == SQLITE_ROW) {
        currentTitle = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 0));
        currentAuthor = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 1));
    }

    sqlite3_finalize(selectStmt);

    std::string newTitle, newAuthor;
    std::cout
        << "Enter the new title of the book (or press Enter to keep it unchanged, current title: "
        << currentTitle << "): ";
    std::cin.ignore();
    std::getline(std::cin, newTitle);

    if (newTitle.empty()) {
        newTitle = currentTitle;  // Keep the current title if the user presses Enter
    }

    std::cout
        << "Enter the new author of the book (or press Enter to keep it unchanged, current author: "
        << currentAuthor << "): ";
    std::getline(std::cin, newAuthor);

    if (newAuthor.empty()) {
        newAuthor = currentAuthor;  // Keep the current author if the user presses Enter
    }

    // Use parameterized query to update the book
    const char* updateSQL = "UPDATE books SET title = ?, author = ? WHERE id = ?;";
    sqlite3_stmt* updateStmt;

    rc = sqlite3_prepare_v2(db, updateSQL, -1, &updateStmt, nullptr);
    if (rc != SQLITE_OK) {
        handleSqliteError(db, "prepare statement");
        return;
    }

    sqlite3_bind_text(updateStmt, 1, newTitle.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(updateStmt, 2, newAuthor.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(updateStmt, 3, bookId);

    rc = sqlite3_step(updateStmt);
    if (rc != SQLITE_DONE) {
        handleSqliteError(db, "execute statement");
    } else {
        std::cout << "Book updated successfully.\n";
    }

    sqlite3_finalize(updateStmt);
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

// Function to validate user input as an integer
int getValidIntegerInput() {
    int input;
    while (!(std::cin >> input)) {
        std::cin.clear();  // Clear the error flag
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                        '\n');  // Discard invalid input
        std::cout << "Please enter a valid integer: ";
    }
    return input;
}
