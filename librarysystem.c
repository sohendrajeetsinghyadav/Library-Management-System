#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ANSI colors removed for simplicity */

/* Limits */
#define MAX_STR 100
#define MAX_BOOKS 500
#define MAX_MEMBERS 500
#define MAX_ISSUES 1000
#define MAX_DAYS 4
#define FINE_PER_DAY 10.0
#define MAX_BORROW 3

/* File names */
#define FILE_BOOKS "books.dat"
#define FILE_MEMBERS "members.dat"
#define FILE_ISSUES "issues.dat"
#define FILE_META "meta.dat"

/* Structs */
typedef struct
{
    int id;
    char title[MAX_STR];
    char author[MAX_STR];
    char genre[MAX_STR];
    int totalCopies;
    int availableCopies;
    int active; /* 1 = exists, 0 = deleted */
} Book;

typedef struct
{
    int id;
    char name[MAX_STR];
    char email[MAX_STR];
    char phone[MAX_STR];
    int booksIssued;
    int active; /* 1 = exists, 0 = deleted */
} Member;

typedef struct
{
    int issueId;
    int bookId;
    int memberId;
    char issueDate[12];
    char returnDate[12];
    double extraFine;
} Issue;

/* Function Prototypes */
void addFine();
void saveData();
int loadData();

/* Globals */
Book books[MAX_BOOKS];
Member members[MAX_MEMBERS];
Issue issues[MAX_ISSUES];
int bookCount = 0, memberCount = 0, issueCount = 0;
int nextBookId = 1, nextMemberId = 1;

/* ════════════════════════════════════════════════
   File persistence
   ════════════════════════════════════════════════ */

void saveData()
{
    FILE *f;

    /* Save books */
    f = fopen(FILE_BOOKS, "wb");
    if (f)
    {
        fwrite(&bookCount, sizeof(int), 1, f);
        fwrite(books, sizeof(Book), bookCount, f);
        fclose(f);
    }
    else
    {
        printf("[ERROR] Could not save books data.\n");
    }

    /* Save members */
    f = fopen(FILE_MEMBERS, "wb");
    if (f)
    {
        fwrite(&memberCount, sizeof(int), 1, f);
        fwrite(members, sizeof(Member), memberCount, f);
        fclose(f);
    }
    else
    {
        printf("[ERROR] Could not save members data.\n");
    }

    /* Save issues */
    f = fopen(FILE_ISSUES, "wb");
    if (f)
    {
        fwrite(&issueCount, sizeof(int), 1, f);
        fwrite(issues, sizeof(Issue), issueCount, f);
        fclose(f);
    }
    else
    {
        printf("[ERROR] Could not save issues data.\n");
    }

    /* Save meta (ID counters) */
    f = fopen(FILE_META, "wb");
    if (f)
    {
        fwrite(&nextBookId, sizeof(int), 1, f);
        fwrite(&nextMemberId, sizeof(int), 1, f);
        fclose(f);
    }
    else
    {
        printf("[ERROR] Could not save meta data.\n");
    }
}

/* Returns 1 if saved data was found and loaded, 0 if not (fresh start) */
int loadData()
{
    FILE *f;

    f = fopen(FILE_BOOKS, "rb");
    if (!f)
        return 0; /* no saved data — use seed */
    fread(&bookCount, sizeof(int), 1, f);
    fread(books, sizeof(Book), bookCount, f);
    fclose(f);

    f = fopen(FILE_MEMBERS, "rb");
    if (!f)
    {
        bookCount = 0;
        return 0;
    }
    fread(&memberCount, sizeof(int), 1, f);
    fread(members, sizeof(Member), memberCount, f);
    fclose(f);

    f = fopen(FILE_ISSUES, "rb");
    if (!f)
    {
        bookCount = memberCount = 0;
        return 0;
    }
    fread(&issueCount, sizeof(int), 1, f);
    fread(issues, sizeof(Issue), issueCount, f);
    fclose(f);

    f = fopen(FILE_META, "rb");
    if (!f)
    {
        bookCount = memberCount = issueCount = 0;
        return 0;
    }
    fread(&nextBookId, sizeof(int), 1, f);
    fread(&nextMemberId, sizeof(int), 1, f);
    fclose(f);

    return 1;
}

/* ════════════════════════════════════════════════
   UI helpers
   ════════════════════════════════════════════════ */

void printHeader(const char *title)
{
    printf("\n--- %s ---\n", title);
}

void printDivider()
{
    printf("----------------------------------------------\n");
}

void printSuccess(const char *msg) { printf("[OK] %s\n", msg); }
void printError(const char *msg) { printf("[ERROR] %s\n", msg); }
void printWarn(const char *msg) { printf("[WARNING] %s\n", msg); }
void printInfo(const char *msg) { printf("[INFO] %s\n", msg); }

/* ════════════════════════════════════════════════
   Input helpers
   ════════════════════════════════════════════════ */

int getIntInput(const char *prompt)
{
    int n;
    char buf[32];
    while (1)
    {
        printf("%s", prompt);
        if (fgets(buf, sizeof(buf), stdin) && sscanf(buf, "%d", &n) == 1)
            return n;
        printError("Please enter a valid number.");
    }
}

void getStrInput(const char *prompt, char *buf, int maxLen)
{
    printf("%s", prompt);
    fgets(buf, maxLen, stdin);
    buf[strcspn(buf, "\n")] = 0;
    /* trim leading spaces */
    int start = 0;
    while (buf[start] == ' ')
        start++;
    if (start)
        memmove(buf, buf + start, strlen(buf) - start + 1);
}

/* ════════════════════════════════════════════════
   Date helpers
   ════════════════════════════════════════════════ */

void todayStr(char *buf)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    strftime(buf, 12, "%Y-%m-%d", lt);
}

int daysSince(const char *dateStr)
{
    struct tm t = {0};
    sscanf(dateStr, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday);
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    time_t then = mktime(&t);
    if (then == (time_t)-1)
        return 0;
    return (int)(difftime(time(NULL), then) / 86400);
}

/* ════════════════════════════════════════════════
   Lookup helpers
   ════════════════════════════════════════════════ */

Book *findBookById(int id)
{
    for (int i = 0; i < bookCount; i++)
        if (books[i].id == id && books[i].active)
            return &books[i];
    return NULL;
}

Member *findMemberById(int id)
{
    for (int i = 0; i < memberCount; i++)
        if (members[i].id == id && members[i].active)
            return &members[i];
    return NULL;
}

/* Case-insensitive substring search */
int containsCI(const char *hay, const char *needle)
{
    char h[MAX_STR], n[MAX_STR];
    int i;
    for (i = 0; hay[i]; i++)
    {
        h[i] = tolower((unsigned char)hay[i]);
    }
    h[i] = 0;
    for (i = 0; needle[i]; i++)
    {
        n[i] = tolower((unsigned char)needle[i]);
    }
    n[i] = 0;
    return strstr(h, n) != NULL;
}

/* ════════════════════════════════════════════════
   Book functions
   ════════════════════════════════════════════════ */

void addBook()
{
    if (bookCount >= MAX_BOOKS)
    {
        printError("Book limit reached.");
        return;
    }
    printHeader("Add New Book");
    Book *b = &books[bookCount];
    memset(b, 0, sizeof(Book));
    b->id = nextBookId++;
    b->active = 1;
    getStrInput("Title: ", b->title, MAX_STR);
    getStrInput("Author: ", b->author, MAX_STR);
    getStrInput("Genre: ", b->genre, MAX_STR);
    if (strlen(b->title) == 0)
    {
        printError("Title cannot be empty.");
        nextBookId--;
        return;
    }
    b->totalCopies = getIntInput("Copies: ");
    if (b->totalCopies <= 0)
    {
        printError("Copies must be > 0.");
        nextBookId--;
        return;
    }
    b->availableCopies = b->totalCopies;
    bookCount++;
    saveData();
    printSuccess("Book added successfully!");
}

void viewBooks()
{
    printHeader("All Books");
    int found = 0;
    printf("%-4s  %-30s  %-22s  %-14s  %s\n",
           "ID", "Title", "Author", "Genre", "Available");
    printDivider();
    for (int i = 0; i < bookCount; i++)
    {
        if (!books[i].active)
            continue;
        found = 1;
        printf("%-4d  %-30s  %-22s  %-14s  %d/%d\n",
               books[i].id, books[i].title, books[i].author, books[i].genre,
               books[i].availableCopies, books[i].totalCopies);
    }
    if (!found)
        printWarn("No books found.");
}

void searchBooks()
{
    printHeader("Search Books");
    char query[MAX_STR];
    getStrInput("Search (title / author / genre): ", query, MAX_STR);
    if (strlen(query) == 0)
    {
        printError("Search query is empty.");
        return;
    }
    int found = 0;
    printf("\n%-4s  %-30s  %-22s  %-14s  %s\n",
           "ID", "Title", "Author", "Genre", "Available");
    printDivider();
    for (int i = 0; i < bookCount; i++)
    {
        if (!books[i].active)
            continue;
        if (containsCI(books[i].title, query) ||
            containsCI(books[i].author, query) ||
            containsCI(books[i].genre, query))
        {
            found = 1;
            printf("%-4d  %-30s  %-22s  %-14s  %d/%d\n",
                   books[i].id, books[i].title, books[i].author, books[i].genre,
                   books[i].availableCopies, books[i].totalCopies);
        }
    }
    if (!found)
        printWarn("No books matched your search.");
}

void deleteBook()
{
    printHeader("Delete Book");
    viewBooks();
    int id = getIntInput("\nEnter Book ID to delete (0 to cancel): ");
    if (id == 0)
        return;
    Book *b = findBookById(id);
    if (!b)
    {
        printError("Book not found.");
        return;
    }
    if (b->availableCopies < b->totalCopies)
    {
        printWarn("Some copies of this book are currently issued. Cannot delete.");
        return;
    }
    char confirm[8];
    printf("Delete \"%s\"? (yes/no): ", b->title);
    fgets(confirm, sizeof(confirm), stdin);
    if (strncmp(confirm, "yes", 3) == 0)
    {
        b->active = 0;
        saveData();
        printSuccess("Book deleted.");
    }
    else
    {
        printInfo("Deletion cancelled.");
    }
}

/* ════════════════════════════════════════════════
   Member functions
   ════════════════════════════════════════════════ */

void addMember()
{
    if (memberCount >= MAX_MEMBERS)
    {
        printError("Member limit reached.");
        return;
    }
    printHeader("Add New Member");
    Member *m = &members[memberCount];
    memset(m, 0, sizeof(Member));
    m->id = nextMemberId++;
    m->active = 1;
    getStrInput("Name: ", m->name, MAX_STR);
    if (strlen(m->name) == 0)
    {
        printError("Name cannot be empty.");
        nextMemberId--;
        return;
    }
    getStrInput("Email: ", m->email, MAX_STR);
    getStrInput("Phone: ", m->phone, MAX_STR);
    m->booksIssued = 0;
    memberCount++;
    saveData();
    printSuccess("Member added successfully!");
}

void viewMembers()
{
    printHeader("All Members");
    int found = 0;
    printf("%-4s  %-22s  %-28s  %-14s  %s\n",
           "ID", "Name", "Email", "Phone", "Issued");
    printDivider();
    for (int i = 0; i < memberCount; i++)
    {
        if (!members[i].active)
            continue;
        found = 1;
        printf("%-4d  %-22s  %-28s  %-14s  %d\n",
               members[i].id, members[i].name,
               members[i].email, members[i].phone,
               members[i].booksIssued);
    }
    if (!found)
        printWarn("No members found.");
}

void searchMembers()
{
    printHeader("Search Members");
    char query[MAX_STR];
    getStrInput("Search (name / email / phone): ", query, MAX_STR);
    if (strlen(query) == 0)
    {
        printError("Search query is empty.");
        return;
    }
    int found = 0;
    printf("\n%-4s  %-22s  %-28s  %-14s  %s\n",
           "ID", "Name", "Email", "Phone", "Issued");
    printDivider();
    for (int i = 0; i < memberCount; i++)
    {
        if (!members[i].active)
            continue;
        if (containsCI(members[i].name, query) ||
            containsCI(members[i].email, query) ||
            containsCI(members[i].phone, query))
        {
            found = 1;
            printf("%-4d  %-22s  %-28s  %-14s  %d\n",
                   members[i].id, members[i].name,
                   members[i].email, members[i].phone,
                   members[i].booksIssued);
        }
    }
    if (!found)
        printWarn("No members matched your search.");
}

void deleteMember()
{
    printHeader("Delete Member");
    viewMembers();
    int id = getIntInput("\nEnter Member ID to delete (0 to cancel): ");
    if (id == 0)
        return;
    Member *m = findMemberById(id);
    if (!m)
    {
        printError("Member not found.");
        return;
    }
    if (m->booksIssued > 0)
    {
        printWarn("Member still has books issued. Cannot delete.");
        return;
    }
    char confirm[8];
    printf("Delete member \"%s\"? (yes/no): ", m->name);
    fgets(confirm, sizeof(confirm), stdin);
    if (strncmp(confirm, "yes", 3) == 0)
    {
        m->active = 0;
        saveData();
        printSuccess("Member deleted.");
    }
    else
    {
        printInfo("Deletion cancelled.");
    }
}

/* ════════════════════════════════════════════════
   Issue / Return functions
   ════════════════════════════════════════════════ */

void issueBook()
{
    printHeader("Issue Book");

    /* show available books */
    printf("\nAvailable Books:\n");
    printf("%-4s  %-30s  %-22s  Copies\n", "ID", "Title", "Author");
    printDivider();
    for (int i = 0; i < bookCount; i++)
    {
        if (!books[i].active || books[i].availableCopies <= 0)
            continue;
        printf("%-4d  %-30s  %-22s  %d\n",
               books[i].id, books[i].title, books[i].author, books[i].availableCopies);
    }

    int mid = getIntInput("\nMember ID: ");
    int bid = getIntInput("Book ID: ");

    Member *m = findMemberById(mid);
    Book *b = findBookById(bid);

    if (!m)
    {
        printError("Member not found.");
        return;
    }
    if (!b)
    {
        printError("Book not found.");
        return;
    }
    if (m->booksIssued >= MAX_BORROW)
    {
        printf("[ERROR] Borrow limit (%d books) reached for %s.\n", MAX_BORROW, m->name);
        return;
    }
    if (b->availableCopies <= 0)
    {
        printf("[ERROR] No copies of \"%s\" are available.\n", b->title);
        return;
    }

    /* Check if member already has THIS book */
    for (int i = 0; i < issueCount; i++)
    {
        if (issues[i].memberId == mid && issues[i].bookId == bid &&
            strcmp(issues[i].returnDate, "PENDING") == 0)
        {
            printf("[ERROR] %s already has \"%s\" issued.\n", m->name, b->title);
            return;
        }
    }

    Issue *iss = &issues[issueCount];
    iss->extraFine = 0.0;
    iss->issueId = issueCount + 1;
    iss->memberId = mid;
    iss->bookId = bid;
    todayStr(iss->issueDate);
    strcpy(iss->returnDate, "PENDING");

    b->availableCopies--;
    m->booksIssued++;
    issueCount++;

    saveData();

    printf("\n[OK] Issued \"%s\" to %s\n", b->title, m->name);
    printf("Issue ID: %d | Due in %d days\n", issueCount, MAX_DAYS);
}

double computeFine(const char *issueDate)
{
    int days = daysSince(issueDate);
    if (days > MAX_DAYS)
        return (days - MAX_DAYS) * FINE_PER_DAY;
    return 0.0;
}

void returnBook()
{
    printHeader("Return Book");
    int id = getIntInput("  Issue ID: ");
    if (id <= 0 || id > issueCount)
    {
        printError("Invalid Issue ID.");
        return;
    }

    Issue *iss = &issues[id - 1];
    if (strcmp(iss->returnDate, "PENDING") != 0)
    {
        printWarn("This book has already been returned.");
        return;
    }

    Book *b = findBookById(iss->bookId);
    Member *m = findMemberById(iss->memberId);

    double fine = computeFine(iss->issueDate) + iss->extraFine;
    printf("\nBook: %s\nMember: %s\nIssued: %s\n",
           b ? b->title : "(deleted)",
           m ? m->name : "(deleted)",
           iss->issueDate);

    if (fine > 0.0)
    {
        printf("\n[WARNING] Overdue fine: Rs %.2f\n", fine);
    }
    else
    {
        printf("\n[OK] Returned on time. No fine.\n");
    }

    todayStr(iss->returnDate);
    if (b)
        b->availableCopies++;
    if (m)
        m->booksIssued--;

    saveData();
    printSuccess("Book returned successfully.");
}

void viewAllIssues()
{
    printHeader("All Issued Books");
    int found = 0;
    printf("%-5s  %-28s  %-20s  %-12s  %-12s  %s\n",
           "IssID", "Book", "Member", "Issued", "Due/Return", "Fine");
    printDivider();
    for (int i = 0; i < issueCount; i++)
    {
        Issue *iss = &issues[i];
        Book *b = findBookById(iss->bookId);
        Member *m = findMemberById(iss->memberId);

        /* compute due date string */
        char dueOrReturn[20];
        if (strcmp(iss->returnDate, "PENDING") == 0)
        {
            int days = daysSince(iss->issueDate);
            int remaining = MAX_DAYS - days;
            if (remaining < 0)
            {
                snprintf(dueOrReturn, sizeof(dueOrReturn), "OVERDUE %dd", -remaining);
            }
            else
            {
                snprintf(dueOrReturn, sizeof(dueOrReturn), "Due in %dd", remaining);
            }
        }
        else
        {
            snprintf(dueOrReturn, sizeof(dueOrReturn), "%s", iss->returnDate);
        }

        double fine = ((strcmp(iss->returnDate, "PENDING") == 0) ? computeFine(iss->issueDate) : 0.0) + iss->extraFine;

        found = 1;
        printf("%-5d  %-28s  %-20s  %-12s  %-12s  %.2f\n",
               iss->issueId,
               b ? b->title : "(deleted)",
               m ? m->name : "(deleted)",
               iss->issueDate,
               dueOrReturn,
               fine);
    }
    if (!found)
        printWarn("No issues recorded.");
}

void checkFine()
{
    printHeader("Check Fine");
    int id = getIntInput("  Issue ID: ");
    if (id <= 0 || id > issueCount)
    {
        printError("Invalid Issue ID.");
        return;
    }
    Issue *iss = &issues[id - 1];
    Book *b = findBookById(iss->bookId);
    Member *m = findMemberById(iss->memberId);

    printf("\nIssue ID: %d\n", iss->issueId);
    printf("Book: %s\n", b ? b->title : "(deleted)");
    printf("Member: %s\n", m ? m->name : "(deleted)");
    printf("Issued: %s\n", iss->issueDate);

    if (strcmp(iss->returnDate, "PENDING") != 0)
    {
        printf("Returned: %s\n", iss->returnDate);
        printInfo("No fine - already returned.");
        return;
    }

    int days = daysSince(iss->issueDate);
    double fine = computeFine(iss->issueDate) + iss->extraFine;
    printf("Days out: %d / %d allowed\n", days, MAX_DAYS);
    if (fine > 0.0)
        printf("Fine: Rs %.2f\n", fine);
    else
        printf("Fine: None (within limit)\n");
}

void addFine()
{
    printHeader("Add Extra Fine");

    int id = getIntInput("  Issue ID: ");
    if (id <= 0 || id > issueCount)
    {
        printError("Invalid Issue ID.");
        return;
    }

    Issue *iss = &issues[id - 1];

    double fine;
    printf("Enter extra fine amount (Rs): ");
    scanf("%lf", &fine);
    getchar(); /* clear buffer */

    if (fine <= 0)
    {
        printError("Fine must be greater than 0.");
        return;
    }

    iss->extraFine += fine;

    saveData();
    printSuccess("Extra fine added successfully.");
}

/* ════════════════════════════════════════════════
   Sub-menus
   ════════════════════════════════════════════════ */

void bookMenu()
{
    int ch;
    do
    {
        printHeader("Books Menu");
        printf("1. Add Book\n"
               "2. View All Books\n"
               "3. Search Books\n"
               "4. Delete Book\n"
               "0. Back\n");
        printDivider();
        ch = getIntInput("Choice: ");
        switch (ch)
        {
        case 1:
            addBook();
            break;
        case 2:
            viewBooks();
            break;
        case 3:
            searchBooks();
            break;
        case 4:
            deleteBook();
            break;
        case 0:
            break;
        default:
            printError("Invalid choice.");
        }
    } while (ch != 0);
}

void memberMenu()
{
    int ch;
    do
    {
        printHeader("Members Menu");
        printf("1. Add Member\n"
               "2. View All Members\n"
               "3. Search Members\n"
               "4. Delete Member\n"
               "0. Back\n");
        printDivider();
        ch = getIntInput("Choice: ");
        switch (ch)
        {
        case 1:
            addMember();
            break;
        case 2:
            viewMembers();
            break;
        case 3:
            searchMembers();
            break;
        case 4:
            deleteMember();
            break;
        case 0:
            break;
        default:
            printError("Invalid choice.");
        }
    } while (ch != 0);
}

void issueMenu()
{
    int ch;
    do
    {
        printHeader("Issue / Return Menu");
        printf("1. Issue Book\n"
               "2. Return Book\n"
               "3. View All Issues\n"
               "4. Check Fine\n"
               "5. Add Extra Fine\n"
               "0. Back\n");
        printDivider();
        ch = getIntInput("Choice: ");
        switch (ch)
        {
        case 1:
            issueBook();
            break;
        case 2:
            returnBook();
            break;
        case 3:
            viewAllIssues();
            break;
        case 4:
            checkFine();
            break;
        case 5:
            addFine();
            break;
        case 0:
            break;
        default:
            printError("Invalid choice.");
        }
    } while (ch != 0);
}

/* ════════════════════════════════════════════════
   Main menu
   ════════════════════════════════════════════════ */

void mainMenu()
{
    int ch;
    do
    {
        printf("\n--- LIBRARY MANAGEMENT SYSTEM ---\n");
        printf("1. Books\n"
               "2. Members\n"
               "3. Issue / Return\n"
               "0. Exit\n");
        printDivider();
        ch = getIntInput("Choice: ");
        switch (ch)
        {
        case 1:
            bookMenu();
            break;
        case 2:
            memberMenu();
            break;
        case 3:
            issueMenu();
            break;
        case 0:
            saveData();
            printf("\nGoodbye!\n\n");
            break;
        default:
            printError("Invalid choice.");
        }
    } while (ch != 0);
}

/* ════════════════════════════════════════════════
   Seed data
   ════════════════════════════════════════════════ */

void seedData()
{
    /* 120 Books */
    Book seedBooks[] = {
        {1, "The C Programming Language", "Dennis Ritchie & Brian Kernighan", "Programming", 5, 5, 1},
        {2, "Data Structures & Algorithms", "Mark Allen Weiss", "CS", 4, 4, 1},
        {3, "Operating System Concepts", "Abraham Silberschatz", "CS", 3, 3, 1},
        {4, "Computer Networks", "Andrew Tanenbaum", "CS", 2, 2, 1},
        {5, "Database System Concepts", "Henry F. Korth", "CS", 3, 3, 1},
        {6, "Introduction to Algorithms", "Cormen, Leiserson, Rivest", "CS", 4, 4, 1},
        {7, "Clean Code", "Robert C. Martin", "Programming", 3, 3, 1},
        {8, "The Pragmatic Programmer", "Hunt & Thomas", "Programming", 2, 2, 1},
        {9, "Design Patterns", "Gang of Four", "Programming", 2, 2, 1},
        {10, "Artificial Intelligence: A Modern Approach", "Stuart Russell", "AI", 3, 3, 1},
        {11, "Python Crash Course", "Eric Matthes", "Programming", 5, 5, 1},
        {12, "Deep Learning", "Ian Goodfellow", "AI", 2, 2, 1},
        {13, "The Mythical Man-Month", "Frederick P. Brooks", "Software Eng", 2, 2, 1},
        {14, "You Don't Know JS", "Kyle Simpson", "Programming", 3, 3, 1},
        {15, "Structure & Interpretation of CS", "Abelson & Sussman", "CS", 2, 2, 1},
        {16, "To Kill a Mockingbird", "Harper Lee", "Classic Fiction", 4, 5, 1},
        {17, "1984", "George Orwell", "Dystopian", 5, 5, 1},
        {18, "The Great Gatsby", "F. Scott Fitzgerald", "Classic Fiction", 4, 4, 1},
        {19, "A Brief History of Time", "Stephen Hawking", "Science", 5, 5, 1},
        {20, "The Hobbit", "J.R.R. Tolkien", "Fantasy", 5, 5, 1},
        {21, "The Alchemist", "Paulo Coelho", "Philosophical Fiction", 4, 4, 1},
        {22, "Pride and Prejudice", "Jane Austen", "Romance", 5, 4, 1},
        {23, "The Da Vinci Code", "Dan Brown", "Thriller", 4, 4, 1},
        {24, "Sapiens: A Brief History of Humankind", "Yuval Noah Harari", "History", 5, 5, 1},
        {25, "The Catcher in the Rye", "J.D. Salinger", "Coming-of-Age", 4, 4, 1},
        {26, "Brave New World", "Aldous Huxley", "Dystopian", 5, 5, 1},
        {27, "Crime and Punishment", "Fyodor Dostoevsky", "Classic Fiction", 5, 5, 1},
        {28, "The Lord of the Rings", "J.R.R. Tolkien", "Fantasy", 5, 5, 1},
        {29, "Harry Potter and the Sorcerer's Stone", "J.K. Rowling", "Fantasy", 5, 5, 1},
        {30, "The Road", "Cormac McCarthy", "Post-Apocalyptic", 4, 4, 1},
        {31, "Moby Dick", "Herman Melville", "Classic Fiction", 4, 4, 1},
        {32, "War and Peace", "Leo Tolstoy", "Historical Fiction", 5, 5, 1},
        {33, "The Shining", "Stephen King", "Horror", 5, 5, 1},
        {34, "Dracula", "Bram Stoker", "Horror", 5, 4, 1},
        {35, "Frankenstein", "Mary Shelley", "Horror", 5, 4, 1},
        {36, "The Kite Runner", "Khaled Hosseini", "Drama", 5, 5, 1},
        {37, "A Thousand Splendid Suns", "Khaled Hosseini", "Drama", 5, 5, 1},
        {38, "The Girl with the Dragon Tattoo", "Stieg Larsson", "Thriller", 5, 5, 1},
        {39, "Gone Girl", "Gillian Flynn", "Thriller", 4, 5, 1},
        {40, "The Silent Patient", "Alex Michaelides", "Thriller", 4, 4, 1},
        {41, "The Hunger Games", "Suzanne Collins", "Dystopian", 5, 5, 1},
        {42, "Catching Fire", "Suzanne Collins", "Dystopian", 5, 5, 1},
        {43, "Mockingjay", "Suzanne Collins", "Dystopian", 5, 5, 1},
        {44, "The Martian", "Andy Weir", "Science Fiction", 5, 5, 1},
        {45, "Dune", "Frank Herbert", "Science Fiction", 5, 5, 1},
        {46, "Foundation", "Isaac Asimov", "Science Fiction", 5, 5, 1},
        {47, "Neuromancer", "William Gibson", "Cyberpunk", 5, 5, 1},
        {48, "Snow Crash", "Neal Stephenson", "Cyberpunk", 5, 5, 1},
        {49, "The Name of the Wind", "Patrick Rothfuss", "Fantasy", 5, 5, 1},
        {50, "The Wise Man's Fear", "Patrick Rothfuss", "Fantasy", 5, 5, 1},
        {51, "Mistborn: The Final Empire", "Brandon Sanderson", "Fantasy", 5, 5, 1},
        {52, "The Way of Kings", "Brandon Sanderson", "Fantasy", 5, 5, 1},
        {53, "Words of Radiance", "Brandon Sanderson", "Fantasy", 5, 5, 1},
        {54, "Oathbringer", "Brandon Sanderson", "Fantasy", 5, 5, 1},
        {55, "Rhythm of War", "Brandon Sanderson", "Fantasy", 5, 5, 1},
        {56, "The Lean Startup", "Eric Ries", "Business", 5, 5, 1},
        {57, "Zero to One", "Peter Thiel", "Business", 5, 5, 1},
        {58, "The Innovator's Dilemma", "Clayton Christensen", "Business", 5, 5, 1},
        {59, "Thinking, Fast and Slow", "Daniel Kahneman", "Psychology", 5, 5, 1},
        {60, "Atomic Habits", "James Clear", "Self-Help", 5, 5, 1},
        {61, "Deep Work", "Cal Newport", "Self-Help", 5, 5, 1},
        {62, "Digital Minimalism", "Cal Newport", "Self-Help", 5, 5, 1},
        {63, "The Power of Habit", "Charles Duhigg", "Self-Help", 5, 5, 1},
        {64, "Man's Search for Meaning", "Viktor Frankl", "Philosophy", 5, 5, 1},
        {65, "Meditations", "Marcus Aurelius", "Philosophy", 5, 5, 1},
        {66, "The Republic", "Plato", "Philosophy", 5, 5, 1},
        {67, "Nicomachean Ethics", "Aristotle", "Philosophy", 5, 5, 1},
        {68, "Beyond Good and Evil", "Friedrich Nietzsche", "Philosophy", 5, 5, 1},
        {69, "Thus Spoke Zarathustra", "Friedrich Nietzsche", "Philosophy", 5, 5, 1},
        {70, "The Prince", "Niccolò Machiavelli", "Political Philosophy", 5, 5, 1},
        {71, "Leviathan", "Thomas Hobbes", "Political Philosophy", 5, 5, 1},
        {72, "On Liberty", "John Stuart Mill", "Political Philosophy", 5, 5, 1},
        {73, "The Wealth of Nations", "Adam Smith", "Economics", 5, 5, 1},
        {74, "Capital", "Karl Marx", "Economics", 5, 5, 1},
        {75, "Freakonomics", "Steven Levitt & Stephen Dubner", "Economics", 5, 5, 1},
        {76, "Beloved", "Toni Morrison", "Classic Fiction", 5, 5, 1},
        {77, "The Handmaid's Tale", "Margaret Atwood", "Dystopian", 5, 5, 1},
        {78, "The Bell Jar", "Sylvia Plath", "Classic Fiction", 4, 5, 1},
        {79, "Invisible Man", "Ralph Ellison", "Classic Fiction", 5, 5, 1},
        {80, "Catch-22", "Joseph Heller", "Satire", 5, 5, 1},
        {81, "Slaughterhouse-Five", "Kurt Vonnegut", "Science Fiction", 5, 5, 1},
        {82, "The Sun Also Rises", "Ernest Hemingway", "Classic Fiction", 4, 5, 1},
        {83, "The Old Man and the Sea", "Ernest Hemingway", "Classic Fiction", 5, 5, 1},
        {84, "Fahrenheit 451", "Ray Bradbury", "Dystopian", 5, 5, 1},
        {85, "Animal Farm", "George Orwell", "Political Satire", 5, 5, 1},
        {86, "The Stranger", "Albert Camus", "Philosophy", 5, 5, 1},
        {87, "Les Misérables", "Victor Hugo", "Historical Fiction", 5, 5, 1},
        {88, "Don Quixote", "Miguel de Cervantes", "Classic Fiction", 5, 5, 1},
        {89, "The Divine Comedy", "Dante Alighieri", "Epic Poetry", 5, 5, 1},
        {90, "The Iliad", "Homer", "Epic Poetry", 5, 5, 1},
        {91, "The Odyssey", "Homer", "Epic Poetry", 5, 5, 1},
        {92, "The Brothers Karamazov", "Fyodor Dostoevsky", "Classic Fiction", 5, 5, 1},
        {93, "Anna Karenina", "Leo Tolstoy", "Classic Fiction", 5, 5, 1},
        {94, "Madame Bovary", "Gustave Flaubert", "Classic Fiction", 5, 5, 1},
        {95, "The Count of Monte Cristo", "Alexandre Dumas", "Adventure", 5, 5, 1},
        {96, "The Three Musketeers", "Alexandre Dumas", "Adventure", 5, 5, 1},
        {97, "Dr. Jekyll and Mr. Hyde", "Robert Louis Stevenson", "Horror", 5, 5, 1},
        {98, "Treasure Island", "Robert Louis Stevenson", "Adventure", 5, 5, 1},
        {99, "Journey to the Center of the Earth", "Jules Verne", "Science Fiction", 5, 5, 1},
        {100, "Twenty Thousand Leagues Under the Seas", "Jules Verne", "Science Fiction", 5, 5, 1},
        {101, "Around the World in Eighty Days", "Jules Verne", "Adventure", 5, 5, 1},
        {102, "The Time Machine", "H.G. Wells", "Science Fiction", 5, 5, 1},
        {103, "The War of the Worlds", "H.G. Wells", "Science Fiction", 5, 5, 1},
        {104, "The Invisible Man", "H.G. Wells", "Science Fiction", 5, 5, 1},
        {105, "Brave New World Revisited", "Aldous Huxley", "Essays", 4, 5, 1},
        {106, "The Prophet", "Kahlil Gibran", "Philosophy", 5, 5, 1},
        {107, "The Art of War", "Sun Tzu", "Strategy", 5, 5, 1},
        {108, "Tao Te Ching", "Lao Tzu", "Philosophy", 5, 5, 1},
        {109, "Bhagavad Gita", "Vyasa", "Philosophy", 5, 5, 1},
        {110, "The Quran", "Multiple Translators", "Religion", 5, 5, 1},
        {111, "The Bible", "Multiple Authors", "Religion", 5, 5, 1},
        {112, "The Torah", "Multiple Authors", "Religion", 5, 5, 1},
        {113, "The Upanishads", "Ancient Sages", "Philosophy", 5, 5, 1},
        {114, "Meditations on First Philosophy", "René Descartes", "Philosophy", 5, 5, 1},
        {115, "Critique of Pure Reason", "Immanuel Kant", "Philosophy", 5, 5, 1},
        {116, "Being and Time", "Martin Heidegger", "Philosophy", 5, 5, 1},
        {117, "The Communist Manifesto", "Karl Marx & Friedrich Engels", "Political Philosophy", 5, 5, 1},
        {118, "Democracy in America", "Alexis de Tocqueville", "Political Philosophy", 5, 5, 1},
        {119, "On the Origin of Species", "Charles Darwin", "Science", 5, 5, 1},
        {120, "The Double Helix", "James D. Watson", "Science", 5, 5, 1},

    };
    int n = sizeof(seedBooks) / sizeof(seedBooks[0]);
    for (int i = 0; i < n; i++)
    {
        seedBooks[i].id = nextBookId++;
        books[bookCount++] = seedBooks[i];
    }

    /* 240 Members */
    Member seedMembers[] = {
        {1, "Abhinav Sharma", "abhinav.sharma01@gmail.com", "9123456781", 0, 1},
        {2, "Rithvik Roshan", "rithvik.roshan.hr@gmail.com", "9234567812", 0, 1},
        {3, "Rohit Sarin", "rohit.sarin07@yahoo.com", "9345678123", 0, 1},
        {4, "Aditya Roy Kapoor", "aditya.kapoor.pro@gmail.com", "9456781234", 0, 1},
        {5, "Viraj Kohli", "viraj.kohli18@gmail.com", "9567812345", 0, 1},
        {6, "Vikrant Kaushik", "vikrant.kaushik.vk@gmail.com", "9678923456", 0, 1},
        {7, "Varun Sobhati", "varun.sobhati90@gmail.com", "9789034567", 0, 1},
        {8, "Karan Rahul", "karan.rahul.kr@gmail.com", "9890145678", 0, 1},
        {9, "Aditya Sehgal", "aditya.sehgal21@gmail.com", "9901256789", 0, 1},
        {10, "Vikram Kaushik", "vikram.kaushik77@gmail.com", "9012367890", 0, 1},
        {11, "Rashi Pareta", "rashi@example.com", "9876543209", 0, 1},
        {12, "Manish Sharma", "manish@example.com", "9876543208", 0, 1},
        {13, "Smriti Singh", "smriti@example.com", "9876543207", 0, 1},
        {14, "Rakshita Mehta", "rakshita@example.com", "9876543206", 0, 1},
        {15, "Sohendrajeet Singh", "sohendrajeet@example.com", "9876543205", 0, 1},
        {16, "Aarav Sharma", "aarav@example.com", "9876543210", 0, 1},
        {17, "Priya Verma", "priya@example.com", "9876543211", 0, 1},
        {18, "Rohan Gupta", "rohan@example.com", "9876543212", 0, 1},
        {19, "Sneha Patel", "sneha@example.com", "9876543213", 0, 1},
        {20, "Vikram Singh", "vikram@example.com", "9876543214", 0, 1},
        {21, "Meera Nair", "meera@example.com", "9876543215", 0, 1},
        {22, "Arjun Mehta", "arjun@example.com", "9876543216", 0, 1},
        {23, "Kavya Reddy", "kavya@example.com", "9876543217", 0, 1},
        {24, "Ishaan Joshi", "ishaan@example.com", "9876543218", 0, 1},
        {25, "Tanvi Kulkarni", "tanvi@example.com", "9876543219", 0, 1},
        {26, "Aditya Rao", "aditya@example.com", "9876543220", 0, 1},
        {27, "Ananya Desai", "ananya@example.com", "9876543221", 0, 1},
        {28, "Kabir Malhotra", "kabir@example.com", "9876543222", 0, 1},
        {29, "Sanya Kapoor", "sanya@example.com", "9876543223", 0, 1},
        {30, "Raghav Chatterjee", "raghav@example.com", "9876543224", 0, 1},
        {31, "Aman Verma", "aman.verma01@gmail.com", "9123456780", 0, 1},
        {32, "Riya Mehta", "riya.mehta22@gmail.com", "9234567811", 0, 1},
        {33, "Siddharth Singh", "siddharth.singh33@gmail.com", "9345678122", 0, 1},
        {34, "Neha Kapoor", "neha.kapoor44@gmail.com", "9456781233", 0, 1},
        {35, "Arjun Malhotra", "arjun.malhotra55@gmail.com", "9567812344", 0, 1},
        {36, "Priya Sharma", "priya.sharma66@gmail.com", "9678923455", 0, 1},
        {37, "Kunal Bhatia", "kunal.bhatia77@gmail.com", "9789034566", 0, 1},
        {38, "Sneha Rathi", "sneha.rathi88@gmail.com", "9890145677", 0, 1},
        {39, "Rahul Chauhan", "rahul.chauhan99@gmail.com", "9901256788", 0, 1},
        {40, "Ishita Roy", "ishita.roy10@gmail.com", "9012367899", 0, 1},
        {41, "Manish Gupta", "manish.gupta11@gmail.com", "9123478901", 0, 1},
        {42, "Ananya Das", "ananya.das12@gmail.com", "9234589012", 0, 1},
        {43, "Vivek Nair", "vivek.nair13@gmail.com", "9345690123", 0, 1},
        {44, "Meera Joshi", "meera.joshi14@gmail.com", "9456701234", 0, 1},
        {45, "Raghav Tiwari", "raghav.tiwari15@gmail.com", "9567812346", 0, 1},
        {46, "Divya Saxena", "divya.saxena16@gmail.com", "9678923457", 0, 1},
        {47, "Nikhil Arora", "nikhil.arora17@gmail.com", "9789034568", 0, 1},
        {48, "Shreya Patel", "shreya.patel18@gmail.com", "9890145679", 0, 1},
        {49, "Varun Khanna", "varun.khanna19@gmail.com", "9901256780", 0, 1},
        {50, "Tanvi Agarwal", "tanvi.agarwal20@gmail.com", "9012367891", 0, 1},
        {51, "Aarav Sharma", "aarav.sharma51@gmail.com", "9123456701", 0, 1},
        {52, "Ishaan Mehta", "ishaan.mehta52@gmail.com", "9234567812", 0, 1},
        {53, "Kabir Nanda", "kabir.nanda53@gmail.com", "9345678123", 0, 1},
        {54, "Anaya Kapoor", "anaya.kapoor54@gmail.com", "9456781234", 0, 1},
        {55, "Vihaan Joshi", "vihaan.joshi55@gmail.com", "9567812345", 0, 1},
        {56, "Myra Bhatia", "myra.bhatia56@gmail.com", "9678923456", 0, 1},
        {57, "Advait Malhotra", "advait.malhotra57@gmail.com", "9789034567", 0, 1},
        {58, "Aanya Rathi", "aanya.rathi58@gmail.com", "9890145678", 0, 1},
        {59, "Reyansh Chauhan", "reyansh.chauhan59@gmail.com", "9901256789", 0, 1},
        {60, "Sara Roy", "sara.roy60@gmail.com", "9012367890", 0, 1},
        {61, "Arnav Gupta", "arnav.gupta61@gmail.com", "9123478902", 0, 1},
        {62, "Tanishq Das", "tanishq.das62@gmail.com", "9234589013", 0, 1},
        {63, "Pari Nair", "pari.nair63@gmail.com", "9345690124", 0, 1},
        {64, "Vivaan Joshi", "vivaan.joshi64@gmail.com", "9456701235", 0, 1},
        {65, "Rudra Tiwari", "rudra.tiwari65@gmail.com", "9567812347", 0, 1},
        {66, "Kiara Saxena", "kiara.saxena66@gmail.com", "9678923458", 0, 1},
        {67, "Aryan Arora", "aryan.arora67@gmail.com", "9789034569", 0, 1},
        {68, "Navya Patel", "navya.patel68@gmail.com", "9890145680", 0, 1},
        {69, "Krishna Khanna", "krishna.khanna69@gmail.com", "9901256781", 0, 1},
        {70, "Aadhya Agarwal", "aadhya.agarwal70@gmail.com", "9012367892", 0, 1},
        {71, "Rohan Verma", "rohan.verma71@gmail.com", "9123456703", 0, 1},
        {72, "Saanvi Mehta", "saanvi.mehta72@gmail.com", "9234567814", 0, 1},
        {73, "Dev Nanda", "dev.nanda73@gmail.com", "9345678125", 0, 1},
        {74, "Anvi Kapoor", "anvi.kapoor74@gmail.com", "9456781236", 0, 1},
        {75, "Atharv Joshi", "atharv.joshi75@gmail.com", "9567812348", 0, 1},
        {76, "Prisha Bhatia", "prisha.bhatia76@gmail.com", "9678923459", 0, 1},
        {77, "Aayush Malhotra", "aayush.malhotra77@gmail.com", "9789034570", 0, 1},
        {78, "Siya Rathi", "siya.rathi78@gmail.com", "9890145681", 0, 1},
        {79, "Arjun Chauhan", "arjun.chauhan79@gmail.com", "9901256782", 0, 1},
        {80, "Mira Roy", "mira.roy80@gmail.com", "9012367893", 0, 1},
        {81, "Yash Gupta", "yash.gupta81@gmail.com", "9123478904", 0, 1},
        {82, "Aarohi Das", "aarohi.das82@gmail.com", "9234589015", 0, 1},
        {83, "Kiaan Nair", "kiaan.nair83@gmail.com", "9345690126", 0, 1},
        {84, "Riya Joshi", "riya.joshi84@gmail.com", "9456701237", 0, 1},
        {85, "Vivaan Tiwari", "vivaan.tiwari85@gmail.com", "9567812349", 0, 1},
        {86, "Anika Saxena", "anika.saxena86@gmail.com", "9678923460", 0, 1},
        {87, "Advik Arora", "advik.arora87@gmail.com", "9789034571", 0, 1},
        {88, "Charvi Patel", "charvi.patel88@gmail.com", "9890145682", 0, 1},
        {89, "Aryan Khanna", "aryan.khanna89@gmail.com", "9901256783", 0, 1},
        {90, "Avni Agarwal", "avni.agarwal90@gmail.com", "9012367894", 0, 1},
        {91, "Rudra Verma", "rudra.verma91@gmail.com", "9123456705", 0, 1},
        {92, "Amaira Mehta", "amaira.mehta92@gmail.com", "9234567816", 0, 1},
        {93, "Kabir Nanda", "kabir.nanda93@gmail.com", "9345678127", 0, 1},
        {94, "Anaya Kapoor", "anaya.kapoor94@gmail.com", "9456781238", 0, 1},
        {95, "Vihaan Joshi", "vihaan.joshi95@gmail.com", "9567812350", 0, 1},
        {96, "Myra Bhatia", "myra.bhatia96@gmail.com", "9678923461", 0, 1},
        {97, "Advait Malhotra", "advait.malhotra97@gmail.com", "9789034572", 0, 1},
        {98, "Aanya Rathi", "aanya.rathi98@gmail.com", "9890145683", 0, 1},
        {99, "Reyansh Chauhan", "reyansh.chauhan99@gmail.com", "9901256784", 0, 1},
        {100, "Sara Roy", "sara.roy100@gmail.com", "9012367895", 0, 1},
        {101, "Arnav Gupta", "arnav.gupta101@gmail.com", "9123478906", 0, 1},
        {102, "Tanishq Das", "tanishq.das102@gmail.com", "9234589017", 0, 1},
        {103, "Pari Nair", "pari.nair103@gmail.com", "9345690128", 0, 1},
        {104, "Vivaan Joshi", "vivaan.joshi104@gmail.com", "9456701239", 0, 1},
        {105, "Rudra Tiwari", "rudra.tiwari105@gmail.com", "9567812351", 0, 1},
        {106, "Kiara Saxena", "kiara.saxena106@gmail.com", "9678923462", 0, 1},
        {107, "Aryan Arora", "aryan.arora107@gmail.com", "9789034573", 0, 1},
        {108, "Navya Patel", "navya.patel108@gmail.com", "9890145684", 0, 1},
        {109, "Krishna Khanna", "krishna.khanna109@gmail.com", "9901256785", 0, 1},
        {110, "Aadhya Agarwal", "aadhya.agarwal110@gmail.com", "9012367896", 0, 1},
        {111, "Rohan Verma", "rohan.verma111@gmail.com", "9123456707", 0, 1},
        {112, "Saanvi Mehta", "saanvi.mehta112@gmail.com", "9234567818", 0, 1},
        {113, "Dev Nanda", "dev.nanda113@gmail.com", "9345678129", 0, 1},
        {114, "Anvi Kapoor", "anvi.kapoor114@gmail.com", "9456781240", 0, 1},
        {115, "Atharv Joshi", "atharv.joshi115@gmail.com", "9567812352", 0, 1},
        {116, "Prisha Bhatia", "prisha.bhatia116@gmail.com", "9678923463", 0, 1},
        {117, "Aayush Malhotra", "aayush.malhotra117@gmail.com", "9789034574", 0, 1},
        {118, "Siya Rathi", "siya.rathi118@gmail.com", "9890145685", 0, 1},
        {119, "Arjun Chauhan", "arjun.chauhan119@gmail.com", "9901256786", 0, 1},
        {120, "Mira Roy", "mira.roy120@gmail.com", "9012367897", 0, 1},
        {121, "Arsh Askari", "arsh.askari@example.com", "9123456701", 0, 1},
        {122, "Ansh Sharma", "ansh.sharma@example.com", "9123456702", 0, 1},
        {123, "Vansh Agrawal", "vansh.agrawal@example.com", "9123456703", 0, 1},
        {124, "Dhawal Srivastav", "dhawal.srivastav@example.com", "9123456704", 0, 1},
        {125, "Aarushi Verma", "aarushi.verma@example.com", "9123456705", 0, 1},
        {126, "Krishna Pandey", "krishna.pandey@example.com", "9123456706", 0, 1},
        {127, "Arnaw", "arnaw@example.com", "9123456707", 0, 1},
        {128, "Abhiraj Khatri", "abhiraj.khatri@example.com", "9123456708", 0, 1},
        {129, "Prachi Rawat", "prachi.rawat@example.com", "9123456709", 0, 1},
        {130, "Kartik Lal", "kartik.lal@example.com", "9123456710", 0, 1},
        {131, "Priyal Chadha", "priyal.chadha@example.com", "9123456711", 0, 1},
        {132, "Sadulla", "sadulla@example.com", "9123456712", 0, 1},
        {133, "Lakshay Aggarwal", "lakshay.aggarwal@example.com", "9123456713", 0, 1},
        {134, "Kritika", "kritika@example.com", "9123456714", 0, 1},
        {135, "Aman Singh Negi", "aman.negi@example.com", "9123456715", 0, 1},
        {136, "Titiksh Soni", "titiksh.soni@example.com", "9123456716", 0, 1},
        {137, "Sahibzaad Singh", "sahibzaad.singh@example.com", "9123456717", 0, 1},
        {138, "Sayyam Jain", "sayyam.jain@example.com", "9123456718", 0, 1},
        {139, "Arman Chauhan", "arman.chauhan@example.com", "9123456719", 0, 1},
        {140, "Praneet Kumar", "praneet.kumar@example.com", "9123456720", 0, 1},
        {141, "Shiwani Kumari", "shiwani.kumari@example.com", "9123456721", 0, 1},
        {142, "Piyush Shahi", "piyush.shahi@example.com", "9123456722", 0, 1},
        {143, "Komal Kapri", "komal.kapri@example.com", "9123456723", 0, 1},
        {144, "Tanmay Mondal", "tanmay.mondal@example.com", "9123456724", 0, 1},
        {145, "Aditya Raj Thakur", "aditya.thakur@example.com", "9123456725", 0, 1},
        {146, "Gaurik Das", "gaurik.das@example.com", "9123456726", 0, 1},
        {147, "Vaibhav", "vaibhav@example.com", "9123456727", 0, 1},
        {148, "Prateek Sagar", "prateek.sagar@example.com", "9123456728", 0, 1},
        {149, "Amit Kumar", "amit.kumar@example.com", "9123456729", 0, 1},
        {150, "Shivam Kumar Mishra", "shivam.mishra@example.com", "9123456730", 0, 1},
        {151, "Sumit Kumar", "sumit.kumar@example.com", "9123456731", 0, 1},
        {152, "Rudraksh Rastogi", "rudraksh.rastogi@example.com", "9123456732", 0, 1},
        {153, "Tiya Sharma", "tiya.sharma@example.com", "9123456733", 0, 1},
        {154, "Ayush Kumar", "ayush.kumar@example.com", "9123456734", 0, 1},
        {155, "Subham Singh Mehlawat", "subham.mehlawat@example.com", "9123456735", 0, 1},
        {156, "Riya Mehta", "riya.mehta@example.com", "9123456736", 0, 1},
        {157, "Ritesh Kumar Rana", "ritesh.rana@example.com", "9123456737", 0, 1},
        {158, "Surya Bir", "surya.bir@example.com", "9123456738", 0, 1},
        {159, "Rakshit Jain", "rakshit.jain@example.com", "9123456739", 0, 1},
        {160, "Ganesh Mourya", "ganesh.mourya@example.com", "9123456740", 0, 1},
        {161, "Smriti Priyadarshi", "smriti.priyadarshi@example.com", "9123456741", 0, 1},
        {162, "Advika Kumar", "advika.kumar@example.com", "9123456742", 0, 1},
        {163, "Harshdeep Jaiswal", "harshdeep.jaiswal@example.com", "9123456743", 0, 1},
        {164, "Vaibhav", "vaibhav@example.com", "9123456744", 0, 1},
        {165, "Shreyas Thakur", "shreyas.thakur@example.com", "9123456745", 0, 1},
        {166, "Hardik Kumar", "hardik.kumar@example.com", "9123456746", 0, 1},
        {167, "Manjul Chandra", "manjul.chandra@example.com", "9123456747", 0, 1},
        {168, "Nikita Chauhan", "nikita.chauhan@example.com", "9123456748", 0, 1},
        {169, "Lakshay Singh", "lakshay.singh@example.com", "9123456749", 0, 1},
        {170, "Vanshika", "vanshika@example.com", "9123456750", 0, 1},
        {171, "Krutica Chauhaan", "krutica.chauhaan@example.com", "9123456751", 0, 1},
        {172, "Harshvardhan Arya", "harshvardhan.arya@example.com", "9123456752", 0, 1},
        {173, "Kaviyansh Yadav", "kaviyansh.yadav@example.com", "9123456753", 0, 1},
        {174, "Adhya Kulshreshtha", "adhya.kulshreshtha@example.com", "9123456754", 0, 1},
        {175, "Soumya Gupta", "soumya.gupta@example.com", "9123456755", 0, 1},
        {176, "Ishant Moral", "ishant.moral@example.com", "9123456756", 0, 1},
        {177, "Aanya Bhatnagar", "aanya.bhatnagar@example.com", "9123456757", 0, 1},
        {178, "Aarya Gaur", "aarya.gaur@example.com", "9123456758", 0, 1},
        {179, "Raghav Negi", "raghav.negi@example.com", "9123456759", 0, 1},
        {180, "Vivek Kumar Gupta", "vivek.gupta@example.com", "9123456760", 0, 1},
        {181, "Aashita Srivastav", "aashita.srivastav@example.com", "9123456761", 0, 1},
        {182, "Aayush Shrivastav", "aayush.shrivastav@example.com", "9123456762", 0, 1},
        {183, "Saniya Saifi", "saniya.saifi@example.com", "9123456763", 0, 1},
        {184, "Om", "om@example.com", "9123456764", 0, 1},
        {185, "Vishal Kumar", "vishal.kumar@example.com", "9123456765", 0, 1},
        {186, "Shreyansh Jain", "shreyansh.jain@example.com", "9123456766", 0, 1},
        {187, "Prabhu Verma", "prabhu.verma@example.com", "9123456767", 0, 1},
        {188, "Raj Verma", "raj.verma@example.com", "9123456768", 0, 1},
        {189, "Parth Sharma", "parth.sharma@example.com", "9123456769", 0, 1},
        {190, "Shraddha Sharma", "shraddha.sharma@example.com", "9123456770", 0, 1},
        {191, "Aditya Singh", "aditya.singh@example.com", "9123456771", 0, 1},
        {192, "Shivanshi Prashar", "shivanshi.prashar@example.com", "9123456772", 0, 1},
        {193, "Yashvi Singhal", "yashvi.singhal@example.com", "9123456773", 0, 1},
        {194, "Palak Kedia", "palak.kedia@example.com", "9123456774", 0, 1},
        {195, "Krishan Singh", "krishan.singh@example.com", "9123456775", 0, 1},
        {196, "Rohin Sarraf", "rohin.sarraf@example.com", "9123456776", 0, 1},
        {197, "Yash Gupta", "yash.gupta@example.com", "9123456777", 0, 1},
        {198, "Aditi Karn", "aditi.karn@example.com", "9123456778", 0, 1},
        {199, "Sanskriti Sarraf", "sanskriti.sarraf@example.com", "9123456779", 0, 1},
        {200, "Gautam Yadav", "gautam.yadav@example.com", "9123456780", 0, 1},
        {201, "Ravish Kumar", "ravish.kumar@example.com", "9123456781", 0, 1},
        {202, "Krishma Chhimpa", "krishma.chhimpa@example.com", "9123456782", 0, 1},
        {203, "Chirag", "chirag@example.com", "9123456783", 0, 1},
        {204, "Sohendrajeet Singh Yadav", "sohendrajeet.yadav@example.com", "9123456784", 0, 1},
        {205, "Ashad Zaki", "ashad.zaki@example.com", "9123456785", 0, 1},
        {206, "Sushant", "sushant@example.com", "9123456786", 0, 1},
        {207, "Garv Kargeti", "garv.kargeti@example.com", "9123456787", 0, 1},
        {208, "Raj Shekhar", "raj.shekhar@example.com", "9123456788", 0, 1},
        {209, "Madhav Kwatra", "madhav.kwatra@example.com", "9123456789", 0, 1},
        {210, "Brijesh Kumar Mishra", "brijesh.mishra@example.com", "9123456790", 0, 1},
        {211, "Rakshita Sharma", "rakshita.sharma@example.com", "9123456791", 0, 1},
        {212, "Piyush", "piyush@example.com", "9123456792", 0, 1},
        {213, "Ishika Verma", "ishika.verma@example.com", "9123456793", 0, 1},
        {214, "Rachna Garg", "rachna.garg@example.com", "9123456794", 0, 1},
        {215, "Nidhi Mishra", "nidhi.mishra@example.com", "9123456795", 0, 1},
        {216, "Ali Syed Rizvi", "ali.rizvi@example.com", "9123456796", 0, 1},
        {217, "Lucky", "lucky@example.com", "9123456797", 0, 1},
        {218, "Pushkar Singh", "pushkar.singh@example.com", "9123456798", 0, 1},
        {219, "Pratham Rawat", "pratham.rawat@example.com", "9123456799", 0, 1},
        {220, "Vishal Rathore", "vishal.rathore@example.com", "9123456800", 0, 1},
        {221, "Tooshar Bhardwaj", "tooshar.bhardwaj@example.com", "9123456801", 0, 1},
        {222, "Atharva Mendhe", "atharva.mendhe@example.com", "9123456802", 0, 1},
        {223, "Mayank Kumar", "mayank.kumar@example.com", "9123456803", 0, 1},
        {224, "Parth Singhal", "parth.singhal@example.com", "9123456804", 0, 1},
        {225, "Mohd Mubashshir Khan", "mubashshir.khan@example.com", "9123456805", 0, 1},
        {226, "Sanjana Singh", "sanjana.singh@example.com", "9123456806", 0, 1},
        {227, "Prachi Singh Rana", "prachi.rana@example.com", "9123456807", 0, 1},
        {228, "Rashi Pareta", "rashi.pareta@example.com", "9123456808", 0, 1},
        {229, "Manisha Kumari", "manisha.kumari@example.com", "9123456809", 0, 1},
        {230, "Rituraj", "rituraj@example.com", "9123456810", 0, 1},
        {231, "Sangam Kumari", "sangam.kumari@example.com", "9123456811", 0, 1},
        {232, "Manish Kumar", "manish.kumar@example.com", "9123456812", 0, 1},
        {233, "Vaibhav Thakur", "vaibhav.thakur@example.com", "9123456813", 0, 1},
        {234, "Amisha Gupta", "amisha.gupta@example.com", "9123456809", 0, 1},
        {235, "Alankrita Sachi Mishra", "alankrita.mishra@example.com", "9123456810", 0, 1},
        {236, "Om Pandey", "om.pandey@example.com", "9123456811", 0, 1},
        {237, "Kinjal Sharma", "kinjal.sharma@example.com", "9123456812", 0, 1},
        {238, "Chinmay Bhadana", "chinmay.bhadana@example.com", "9123456813", 0, 1},
        {239, "Kartavya Saini", "kartavya.saini@example.com", "9123456814", 0, 1},
        {240, "Lakshay", "lakshay@example.com", "9123456815", 0, 1},

    };
    int m = sizeof(seedMembers) / sizeof(seedMembers[0]);
    for (int i = 0; i < m; i++)
    {
        seedMembers[i].id = nextMemberId++;
        members[memberCount++] = seedMembers[i];
    }
}

/* ════════════════════════════════════════════════
   Entry point
   ════════════════════════════════════════════════ */

int main()
{
    if (!loadData())
    {
    /* No saved data found - load seed data and save it */
        seedData();
        saveData();
        printf("[INFO] First run: seed data loaded and saved.\n");
    }
    else
    {
        printf("[INFO] Data loaded from files.\n");
    }

    mainMenu();
    return 0;
}