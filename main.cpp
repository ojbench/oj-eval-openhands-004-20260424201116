#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

const int MAX_STR_LEN = 65;
const int MAX_KEYWORD_LEN = 65;

struct User {
    char userID[31];
    char password[31];
    char username[31];
    int privilege;
    
    User() {
        memset(userID, 0, sizeof(userID));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
        privilege = 0;
    }
};

struct Book {
    char ISBN[21];
    char bookName[61];
    char author[61];
    char keyword[61];
    double price;
    long long quantity;
    
    Book() {
        memset(ISBN, 0, sizeof(ISBN));
        memset(bookName, 0, sizeof(bookName));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
        price = 0.0;
        quantity = 0;
    }
    
    bool operator<(const Book& other) const {
        return strcmp(ISBN, other.ISBN) < 0;
    }
};

struct Transaction {
    double amount;
    bool isIncome;
};

struct LogEntry {
    char operation[256];
};

class BookstoreSystem {
private:
    map<string, User> users;
    map<string, Book> books;
    vector<Transaction> transactions;
    vector<LogEntry> logs;
    
    vector<pair<string, int>> loginStack;
    map<int, string> selectedBooks;
    
    string userFile = "users.dat";
    string bookFile = "books.dat";
    string transFile = "transactions.dat";
    string logFile = "logs.dat";
    
    bool initialized = false;
    
    void loadData() {
        ifstream uf(userFile, ios::binary);
        if (uf.is_open()) {
            int count;
            uf.read((char*)&count, sizeof(count));
            for (int i = 0; i < count; i++) {
                User u;
                uf.read((char*)&u, sizeof(User));
                users[string(u.userID)] = u;
            }
            uf.close();
            initialized = true;
        }
        
        ifstream bf(bookFile, ios::binary);
        if (bf.is_open()) {
            int count;
            bf.read((char*)&count, sizeof(count));
            for (int i = 0; i < count; i++) {
                Book b;
                bf.read((char*)&b, sizeof(Book));
                books[string(b.ISBN)] = b;
            }
            bf.close();
        }
        
        ifstream tf(transFile, ios::binary);
        if (tf.is_open()) {
            int count;
            tf.read((char*)&count, sizeof(count));
            for (int i = 0; i < count; i++) {
                Transaction t;
                tf.read((char*)&t, sizeof(Transaction));
                transactions.push_back(t);
            }
            tf.close();
        }
        
        ifstream lf(logFile, ios::binary);
        if (lf.is_open()) {
            int count;
            lf.read((char*)&count, sizeof(count));
            for (int i = 0; i < count; i++) {
                LogEntry le;
                lf.read((char*)&le, sizeof(LogEntry));
                logs.push_back(le);
            }
            lf.close();
        }
    }
    
    void saveData() {
        ofstream uf(userFile, ios::binary);
        int count = users.size();
        uf.write((char*)&count, sizeof(count));
        for (auto& p : users) {
            uf.write((char*)&p.second, sizeof(User));
        }
        uf.close();
        
        ofstream bf(bookFile, ios::binary);
        count = books.size();
        bf.write((char*)&count, sizeof(count));
        for (auto& p : books) {
            bf.write((char*)&p.second, sizeof(Book));
        }
        bf.close();
        
        ofstream tf(transFile, ios::binary);
        count = transactions.size();
        tf.write((char*)&count, sizeof(count));
        for (auto& t : transactions) {
            tf.write((char*)&t, sizeof(Transaction));
        }
        tf.close();
        
        ofstream lf(logFile, ios::binary);
        count = logs.size();
        lf.write((char*)&count, sizeof(count));
        for (auto& le : logs) {
            lf.write((char*)&le, sizeof(LogEntry));
        }
        lf.close();
    }
    
    void initialize() {
        if (!initialized) {
            User root;
            strcpy(root.userID, "root");
            strcpy(root.password, "sjtu");
            strcpy(root.username, "root");
            root.privilege = 7;
            users["root"] = root;
            initialized = true;
        }
    }
    
    int getCurrentPrivilege() {
        if (loginStack.empty()) return 0;
        return loginStack.back().second;
    }
    
    string getCurrentUser() {
        if (loginStack.empty()) return "";
        return loginStack.back().first;
    }
    
    bool isValidString(const string& str, const string& type) {
        if (str.empty()) return false;
        
        if (type == "userid" || type == "password") {
            if (str.length() > 30) return false;
            for (char c : str) {
                if (!isalnum(c) && c != '_') return false;
            }
        } else if (type == "username") {
            if (str.length() > 30) return false;
            for (char c : str) {
                if (c < 32 || c > 126) return false;
            }
        } else if (type == "isbn") {
            if (str.length() > 20) return false;
            for (char c : str) {
                if (c < 33 || c > 126) return false;
            }
        } else if (type == "bookname" || type == "author") {
            if (str.length() > 60) return false;
            for (char c : str) {
                if (c < 33 || c > 126 || c == '"') return false;
            }
        } else if (type == "keyword") {
            if (str.length() > 60) return false;
            for (char c : str) {
                if (c < 33 || c > 126 || c == '"') return false;
            }
        }
        
        return true;
    }
    
    bool hasKeyword(const string& keywords, const string& keyword) {
        size_t pos = 0;
        while (pos < keywords.length()) {
            size_t next = keywords.find('|', pos);
            if (next == string::npos) next = keywords.length();
            string kw = keywords.substr(pos, next - pos);
            if (kw == keyword) return true;
            pos = next + 1;
        }
        return false;
    }
    
    bool hasDuplicateKeywords(const string& keywords) {
        set<string> kwSet;
        size_t pos = 0;
        while (pos < keywords.length()) {
            size_t next = keywords.find('|', pos);
            if (next == string::npos) next = keywords.length();
            string kw = keywords.substr(pos, next - pos);
            if (kw.empty() || kwSet.count(kw)) return true;
            kwSet.insert(kw);
            pos = next + 1;
        }
        return false;
    }
    
    void addLog(const string& op) {
        LogEntry le;
        strncpy(le.operation, op.c_str(), 255);
        le.operation[255] = '\0';
        logs.push_back(le);
    }
    
public:
    BookstoreSystem() {
        loadData();
        initialize();
    }
    
    ~BookstoreSystem() {
        saveData();
    }
    
    void processCommand(const string& line) {
        if (line.empty()) return;
        
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "quit" || cmd == "exit") {
            saveData();
            exit(0);
        } else if (cmd == "su") {
            string userID, password;
            iss >> userID;
            if (iss >> password) {
                cmdSu(userID, password);
            } else {
                cmdSu(userID, "");
            }
        } else if (cmd == "logout") {
            cmdLogout();
        } else if (cmd == "register") {
            string userID, password, username;
            iss >> userID >> password;
            getline(iss, username);
            if (!username.empty() && username[0] == ' ') username = username.substr(1);
            cmdRegister(userID, password, username);
        } else if (cmd == "passwd") {
            string userID, pw1, pw2;
            iss >> userID >> pw1;
            if (iss >> pw2) {
                cmdPasswd(userID, pw1, pw2);
            } else {
                cmdPasswd(userID, "", pw1);
            }
        } else if (cmd == "useradd") {
            string userID, password, privilege, username;
            iss >> userID >> password >> privilege;
            getline(iss, username);
            if (!username.empty() && username[0] == ' ') username = username.substr(1);
            cmdUseradd(userID, password, privilege, username);
        } else if (cmd == "delete") {
            string userID;
            iss >> userID;
            cmdDelete(userID);
        } else if (cmd == "show") {
            string param;
            if (!(iss >> param)) {
                cmdShowBooks("");
            } else if (param == "finance") {
                string count;
                if (iss >> count) {
                    cmdShowFinance(count);
                } else {
                    cmdShowFinance("");
                }
            } else {
                cmdShowBooks(param);
            }
        } else if (cmd == "buy") {
            string isbn, quantity;
            iss >> isbn >> quantity;
            cmdBuy(isbn, quantity);
        } else if (cmd == "select") {
            string isbn;
            iss >> isbn;
            cmdSelect(isbn);
        } else if (cmd == "modify") {
            string params;
            getline(iss, params);
            cmdModify(params);
        } else if (cmd == "import") {
            string quantity, cost;
            iss >> quantity >> cost;
            cmdImport(quantity, cost);
        } else if (cmd == "log") {
            cmdLog();
        } else if (cmd == "report") {
            string type;
            iss >> type;
            if (type == "finance") {
                cmdReportFinance();
            } else if (type == "employee") {
                cmdReportEmployee();
            } else {
                cout << "Invalid\n";
            }
        } else {
            cout << "Invalid\n";
        }
    }
    
    void cmdSu(const string& userID, const string& password) {
        if (!isValidString(userID, "userid")) {
            cout << "Invalid\n";
            return;
        }
        if (!password.empty() && !isValidString(password, "password")) {
            cout << "Invalid\n";
            return;
        }
        
        if (users.find(userID) == users.end()) {
            cout << "Invalid\n";
            return;
        }
        
        User& u = users[userID];
        
        if (password.empty()) {
            if (getCurrentPrivilege() <= u.privilege) {
                cout << "Invalid\n";
                return;
            }
        } else {
            if (strcmp(u.password, password.c_str()) != 0) {
                cout << "Invalid\n";
                return;
            }
        }
        
        loginStack.push_back({userID, u.privilege});
        addLog("su " + userID);
    }
    
    void cmdLogout() {
        if (getCurrentPrivilege() < 1) {
            cout << "Invalid\n";
            return;
        }
        
        int stackIdx = loginStack.size() - 1;
        if (selectedBooks.count(stackIdx)) {
            selectedBooks.erase(stackIdx);
        }
        
        loginStack.pop_back();
        addLog("logout");
    }
    
    void cmdRegister(const string& userID, const string& password, const string& username) {
        if (!isValidString(userID, "userid") || !isValidString(password, "password") || !isValidString(username, "username")) {
            cout << "Invalid\n";
            return;
        }
        
        if (users.find(userID) != users.end()) {
            cout << "Invalid\n";
            return;
        }
        
        User u;
        strcpy(u.userID, userID.c_str());
        strcpy(u.password, password.c_str());
        strcpy(u.username, username.c_str());
        u.privilege = 1;
        users[userID] = u;
        
        addLog("register " + userID);
    }
    
    void cmdPasswd(const string& userID, const string& currentPassword, const string& newPassword) {
        if (getCurrentPrivilege() < 1) {
            cout << "Invalid\n";
            return;
        }
        
        if (!isValidString(userID, "userid") || !isValidString(newPassword, "password")) {
            cout << "Invalid\n";
            return;
        }
        if (!currentPassword.empty() && !isValidString(currentPassword, "password")) {
            cout << "Invalid\n";
            return;
        }
        
        if (users.find(userID) == users.end()) {
            cout << "Invalid\n";
            return;
        }
        
        User& u = users[userID];
        
        if (currentPassword.empty()) {
            if (getCurrentPrivilege() != 7) {
                cout << "Invalid\n";
                return;
            }
        } else {
            if (strcmp(u.password, currentPassword.c_str()) != 0) {
                cout << "Invalid\n";
                return;
            }
        }
        
        strcpy(u.password, newPassword.c_str());
        addLog("passwd " + userID);
    }
    
    void cmdUseradd(const string& userID, const string& password, const string& privilege, const string& username) {
        if (getCurrentPrivilege() < 3) {
            cout << "Invalid\n";
            return;
        }
        
        if (!isValidString(userID, "userid") || !isValidString(password, "password") || !isValidString(username, "username")) {
            cout << "Invalid\n";
            return;
        }
        
        if (privilege.length() != 1 || !isdigit(privilege[0])) {
            cout << "Invalid\n";
            return;
        }
        
        int priv = privilege[0] - '0';
        if (priv != 1 && priv != 3 && priv != 7) {
            cout << "Invalid\n";
            return;
        }
        
        if (priv >= getCurrentPrivilege()) {
            cout << "Invalid\n";
            return;
        }
        
        if (users.find(userID) != users.end()) {
            cout << "Invalid\n";
            return;
        }
        
        User u;
        strcpy(u.userID, userID.c_str());
        strcpy(u.password, password.c_str());
        strcpy(u.username, username.c_str());
        u.privilege = priv;
        users[userID] = u;
        
        addLog("useradd " + userID);
    }
    
    void cmdDelete(const string& userID) {
        if (getCurrentPrivilege() < 7) {
            cout << "Invalid\n";
            return;
        }
        
        if (!isValidString(userID, "userid")) {
            cout << "Invalid\n";
            return;
        }
        
        if (users.find(userID) == users.end()) {
            cout << "Invalid\n";
            return;
        }
        
        for (auto& p : loginStack) {
            if (p.first == userID) {
                cout << "Invalid\n";
                return;
            }
        }
        
        users.erase(userID);
        addLog("delete " + userID);
    }
    
    void cmdShowBooks(const string& param) {
        if (getCurrentPrivilege() < 1) {
            cout << "Invalid\n";
            return;
        }
        
        vector<Book> results;
        
        if (param.empty()) {
            for (auto& p : books) {
                results.push_back(p.second);
            }
        } else if (param.substr(0, 6) == "-ISBN=") {
            string isbn = param.substr(6);
            if (!isValidString(isbn, "isbn") || isbn.empty()) {
                cout << "Invalid\n";
                return;
            }
            if (books.find(isbn) != books.end()) {
                results.push_back(books[isbn]);
            }
        } else if (param.substr(0, 7) == "-name=\"") {
            if (param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string name = param.substr(7, param.length() - 8);
            if (!isValidString(name, "bookname") || name.empty()) {
                cout << "Invalid\n";
                return;
            }
            for (auto& p : books) {
                if (strcmp(p.second.bookName, name.c_str()) == 0) {
                    results.push_back(p.second);
                }
            }
        } else if (param.substr(0, 9) == "-author=\"") {
            if (param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string author = param.substr(9, param.length() - 10);
            if (!isValidString(author, "author") || author.empty()) {
                cout << "Invalid\n";
                return;
            }
            for (auto& p : books) {
                if (strcmp(p.second.author, author.c_str()) == 0) {
                    results.push_back(p.second);
                }
            }
        } else if (param.substr(0, 10) == "-keyword=\"") {
            if (param.back() != '"') {
                cout << "Invalid\n";
                return;
            }
            string keyword = param.substr(10, param.length() - 11);
            if (!isValidString(keyword, "keyword") || keyword.empty()) {
                cout << "Invalid\n";
                return;
            }
            if (keyword.find('|') != string::npos) {
                cout << "Invalid\n";
                return;
            }
            for (auto& p : books) {
                if (hasKeyword(p.second.keyword, keyword)) {
                    results.push_back(p.second);
                }
            }
        } else {
            cout << "Invalid\n";
            return;
        }
        
        sort(results.begin(), results.end());
        
        if (results.empty()) {
            cout << "\n";
        } else {
            for (auto& b : results) {
                cout << b.ISBN << "\t" << b.bookName << "\t" << b.author << "\t" 
                     << b.keyword << "\t" << fixed << setprecision(2) << b.price 
                     << "\t" << b.quantity << "\n";
            }
        }
        
        addLog("show books");
    }
    
    void cmdBuy(const string& isbn, const string& quantityStr) {
        if (getCurrentPrivilege() < 1) {
            cout << "Invalid\n";
            return;
        }
        
        if (!isValidString(isbn, "isbn")) {
            cout << "Invalid\n";
            return;
        }
        
        long long quantity;
        try {
            quantity = stoll(quantityStr);
        } catch (...) {
            cout << "Invalid\n";
            return;
        }
        
        if (quantity <= 0) {
            cout << "Invalid\n";
            return;
        }
        
        if (books.find(isbn) == books.end()) {
            cout << "Invalid\n";
            return;
        }
        
        Book& b = books[isbn];
        if (b.quantity < quantity) {
            cout << "Invalid\n";
            return;
        }
        
        double total = b.price * quantity;
        b.quantity -= quantity;
        
        Transaction t;
        t.amount = total;
        t.isIncome = true;
        transactions.push_back(t);
        
        cout << fixed << setprecision(2) << total << "\n";
        addLog("buy " + isbn + " " + quantityStr);
    }
    
    void cmdSelect(const string& isbn) {
        if (getCurrentPrivilege() < 3) {
            cout << "Invalid\n";
            return;
        }
        
        if (!isValidString(isbn, "isbn")) {
            cout << "Invalid\n";
            return;
        }
        
        if (books.find(isbn) == books.end()) {
            Book b;
            strcpy(b.ISBN, isbn.c_str());
            books[isbn] = b;
        }
        
        int stackIdx = loginStack.size() - 1;
        selectedBooks[stackIdx] = isbn;
        
        addLog("select " + isbn);
    }
    
    void cmdModify(const string& params) {
        if (getCurrentPrivilege() < 3) {
            cout << "Invalid\n";
            return;
        }
        
        int stackIdx = loginStack.size() - 1;
        if (!selectedBooks.count(stackIdx)) {
            cout << "Invalid\n";
            return;
        }
        
        string isbn = selectedBooks[stackIdx];
        Book& b = books[isbn];
        
        string newISBN = "";
        string newName = "";
        string newAuthor = "";
        string newKeyword = "";
        double newPrice = -1;
        
        set<string> seen;
        
        size_t pos = 0;
        while (pos < params.length() && params[pos] == ' ') pos++;
        
        while (pos < params.length()) {
            if (params.substr(pos, 6) == "-ISBN=") {
                if (seen.count("ISBN")) {
                    cout << "Invalid\n";
                    return;
                }
                seen.insert("ISBN");
                pos += 6;
                size_t end = pos;
                while (end < params.length() && params[end] != ' ') end++;
                newISBN = params.substr(pos, end - pos);
                if (!isValidString(newISBN, "isbn") || newISBN.empty()) {
                    cout << "Invalid\n";
                    return;
                }
                pos = end;
            } else if (params.substr(pos, 7) == "-name=\"") {
                if (seen.count("name")) {
                    cout << "Invalid\n";
                    return;
                }
                seen.insert("name");
                pos += 7;
                size_t end = params.find('"', pos);
                if (end == string::npos) {
                    cout << "Invalid\n";
                    return;
                }
                newName = params.substr(pos, end - pos);
                if (!isValidString(newName, "bookname") || newName.empty()) {
                    cout << "Invalid\n";
                    return;
                }
                pos = end + 1;
            } else if (params.substr(pos, 9) == "-author=\"") {
                if (seen.count("author")) {
                    cout << "Invalid\n";
                    return;
                }
                seen.insert("author");
                pos += 9;
                size_t end = params.find('"', pos);
                if (end == string::npos) {
                    cout << "Invalid\n";
                    return;
                }
                newAuthor = params.substr(pos, end - pos);
                if (!isValidString(newAuthor, "author") || newAuthor.empty()) {
                    cout << "Invalid\n";
                    return;
                }
                pos = end + 1;
            } else if (params.substr(pos, 10) == "-keyword=\"") {
                if (seen.count("keyword")) {
                    cout << "Invalid\n";
                    return;
                }
                seen.insert("keyword");
                pos += 10;
                size_t end = params.find('"', pos);
                if (end == string::npos) {
                    cout << "Invalid\n";
                    return;
                }
                newKeyword = params.substr(pos, end - pos);
                if (!isValidString(newKeyword, "keyword") || newKeyword.empty()) {
                    cout << "Invalid\n";
                    return;
                }
                if (hasDuplicateKeywords(newKeyword)) {
                    cout << "Invalid\n";
                    return;
                }
                pos = end + 1;
            } else if (params.substr(pos, 7) == "-price=") {
                if (seen.count("price")) {
                    cout << "Invalid\n";
                    return;
                }
                seen.insert("price");
                pos += 7;
                size_t end = pos;
                while (end < params.length() && params[end] != ' ') end++;
                try {
                    newPrice = stod(params.substr(pos, end - pos));
                } catch (...) {
                    cout << "Invalid\n";
                    return;
                }
                if (newPrice < 0) {
                    cout << "Invalid\n";
                    return;
                }
                pos = end;
            } else {
                cout << "Invalid\n";
                return;
            }
            
            while (pos < params.length() && params[pos] == ' ') pos++;
        }
        
        if (seen.empty()) {
            cout << "Invalid\n";
            return;
        }
        
        if (!newISBN.empty()) {
            if (newISBN == isbn) {
                cout << "Invalid\n";
                return;
            }
            if (books.find(newISBN) != books.end()) {
                cout << "Invalid\n";
                return;
            }
        }
        
        if (!newISBN.empty()) {
            books.erase(isbn);
            strcpy(b.ISBN, newISBN.c_str());
            books[newISBN] = b;
            selectedBooks[stackIdx] = newISBN;
        }
        
        if (!newName.empty()) {
            strcpy(books[selectedBooks[stackIdx]].bookName, newName.c_str());
        }
        if (!newAuthor.empty()) {
            strcpy(books[selectedBooks[stackIdx]].author, newAuthor.c_str());
        }
        if (!newKeyword.empty()) {
            strcpy(books[selectedBooks[stackIdx]].keyword, newKeyword.c_str());
        }
        if (newPrice >= 0) {
            books[selectedBooks[stackIdx]].price = newPrice;
        }
        
        addLog("modify");
    }
    
    void cmdImport(const string& quantityStr, const string& costStr) {
        if (getCurrentPrivilege() < 3) {
            cout << "Invalid\n";
            return;
        }
        
        int stackIdx = loginStack.size() - 1;
        if (!selectedBooks.count(stackIdx)) {
            cout << "Invalid\n";
            return;
        }
        
        long long quantity;
        double cost;
        try {
            quantity = stoll(quantityStr);
            cost = stod(costStr);
        } catch (...) {
            cout << "Invalid\n";
            return;
        }
        
        if (quantity <= 0 || cost <= 0) {
            cout << "Invalid\n";
            return;
        }
        
        string isbn = selectedBooks[stackIdx];
        books[isbn].quantity += quantity;
        
        Transaction t;
        t.amount = cost;
        t.isIncome = false;
        transactions.push_back(t);
        
        addLog("import " + quantityStr + " " + costStr);
    }
    
    void cmdShowFinance(const string& countStr) {
        if (getCurrentPrivilege() < 7) {
            cout << "Invalid\n";
            return;
        }
        
        int count;
        if (countStr.empty()) {
            count = transactions.size();
        } else {
            try {
                count = stoi(countStr);
            } catch (...) {
                cout << "Invalid\n";
                return;
            }
        }
        
        if (count == 0) {
            cout << "\n";
            return;
        }
        
        if (count < 0 || count > (int)transactions.size()) {
            cout << "Invalid\n";
            return;
        }
        
        double income = 0, expenditure = 0;
        for (int i = transactions.size() - count; i < (int)transactions.size(); i++) {
            if (transactions[i].isIncome) {
                income += transactions[i].amount;
            } else {
                expenditure += transactions[i].amount;
            }
        }
        
        cout << "+ " << fixed << setprecision(2) << income << " - " << expenditure << "\n";
        addLog("show finance");
    }
    
    void cmdLog() {
        if (getCurrentPrivilege() < 7) {
            cout << "Invalid\n";
            return;
        }
        
        for (auto& le : logs) {
            cout << le.operation << "\n";
        }
    }
    
    void cmdReportFinance() {
        if (getCurrentPrivilege() < 7) {
            cout << "Invalid\n";
            return;
        }
        
        cout << "=== Financial Report ===\n";
        double income = 0, expenditure = 0;
        for (auto& t : transactions) {
            if (t.isIncome) {
                income += t.amount;
            } else {
                expenditure += t.amount;
            }
        }
        cout << "Total Income: " << fixed << setprecision(2) << income << "\n";
        cout << "Total Expenditure: " << expenditure << "\n";
        cout << "Net Profit: " << (income - expenditure) << "\n";
    }
    
    void cmdReportEmployee() {
        if (getCurrentPrivilege() < 7) {
            cout << "Invalid\n";
            return;
        }
        
        cout << "=== Employee Work Report ===\n";
        for (auto& le : logs) {
            cout << le.operation << "\n";
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    BookstoreSystem system;
    
    string line;
    while (getline(cin, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
            line = line.substr(1);
        }
        
        string processed;
        bool lastWasSpace = false;
        for (char c : line) {
            if (c == ' ' || c == '\t') {
                if (!lastWasSpace && !processed.empty()) {
                    processed += ' ';
                    lastWasSpace = true;
                }
            } else {
                processed += c;
                lastWasSpace = false;
            }
        }
        
        if (!processed.empty()) {
            system.processCommand(processed);
        }
    }
    
    return 0;
}
