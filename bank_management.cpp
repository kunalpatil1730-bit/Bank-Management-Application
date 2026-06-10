/*
 * ============================================================
 *   Bank Management Application
 *   Language : C++17
 *   Concepts : OOP (Inheritance, Encapsulation, Polymorphism)
 *              File Handling (binary + text logs)
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <ctime>
#include <algorithm>

// ─────────────────────────────────────────────
//  Utility helpers
// ─────────────────────────────────────────────
namespace utils {

std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause() {
    std::cout << "\n  Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

bool confirm(const std::string& msg) {
    char ch;
    std::cout << "  " << msg << " (y/n): ";
    std::cin >> ch;
    return (ch == 'y' || ch == 'Y');
}

} // namespace utils

// ─────────────────────────────────────────────
//  Transaction record
// ─────────────────────────────────────────────
struct Transaction {
    std::string type;       // DEPOSIT / WITHDRAW / OPEN / CLOSE
    double      amount;
    double      balanceAfter;
    std::string timestamp;
};

// ─────────────────────────────────────────────
//  Base class: Account
// ─────────────────────────────────────────────
class Account {
protected:
    long        accountNumber;
    std::string holderName;
    std::string pin;            // stored as plain text for demo; hash in production
    double      balance;
    std::string accountType;
    std::vector<Transaction> history;

public:
    Account() : accountNumber(0), balance(0.0) {}

    Account(long accNo, const std::string& name,
            const std::string& pin, double initialDeposit,
            const std::string& type)
        : accountNumber(accNo), holderName(name), pin(pin),
          balance(initialDeposit), accountType(type)
    {
        recordTransaction("OPEN", initialDeposit);
    }

    virtual ~Account() {}

    // ── Getters ──────────────────────────────
    long        getAccountNumber() const { return accountNumber; }
    std::string getHolderName()    const { return holderName; }
    double      getBalance()       const { return balance; }
    std::string getAccountType()   const { return accountType; }
    bool        verifyPin(const std::string& p) const { return pin == p; }

    // Used by Bank::loadData to restore a saved account
    void loadFields(long accNo, const std::string& name_,
                    const std::string& pin_, double bal,
                    const std::string& type_) {
        accountNumber = accNo;
        holderName    = name_;
        pin           = pin_;
        balance       = bal;
        accountType   = type_;
    }

    // ── Core operations ──────────────────────
    virtual bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        recordTransaction("DEPOSIT", amount);
        return true;
    }

    virtual bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        recordTransaction("WITHDRAW", amount);
        return true;
    }

    // ── Display ──────────────────────────────
    virtual void displayDetails() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  ┌─────────────────────────────────────┐\n";
        std::cout << "  │         Account Details              │\n";
        std::cout << "  ├─────────────────────────────────────┤\n";
        std::cout << "  │  Account No : " << std::setw(20) << accountNumber << "  │\n";
        std::cout << "  │  Name       : " << std::setw(20) << holderName    << "  │\n";
        std::cout << "  │  Type       : " << std::setw(20) << accountType   << "  │\n";
        std::cout << "  │  Balance    : " << std::setw(19) << balance << " ₹ │\n";
        std::cout << "  └─────────────────────────────────────┘\n";
    }

    void displayHistory() const {
        std::cout << "\n  Transaction History for Account " << accountNumber << "\n";
        std::cout << "  " << std::string(62, '-') << "\n";
        std::cout << "  " << std::left
                  << std::setw(12) << "Type"
                  << std::setw(14) << "Amount (₹)"
                  << std::setw(16) << "Balance (₹)"
                  << "Timestamp\n";
        std::cout << "  " << std::string(62, '-') << "\n";
        for (const auto& t : history) {
            std::cout << "  " << std::left
                      << std::setw(12) << t.type
                      << std::setw(14) << std::fixed << std::setprecision(2) << t.amount
                      << std::setw(16) << t.balanceAfter
                      << t.timestamp << "\n";
        }
    }

    // ── File serialisation ───────────────────
    //  Format (binary-friendly text):
    //  accountNumber|holderName|pin|balance|accountType|txCount
    //  type|amount|balanceAfter|timestamp   (repeated txCount times)
    virtual void serialize(std::ofstream& out) const {
        out << accountNumber << "|" << holderName << "|" << pin << "|"
            << std::fixed << std::setprecision(2) << balance << "|"
            << accountType << "|" << history.size() << "\n";
        for (const auto& t : history)
            out << t.type << "|" << t.amount << "|"
                << t.balanceAfter << "|" << t.timestamp << "\n";
    }

    virtual void deserialize(std::ifstream& in, int txCount) {
        for (int i = 0; i < txCount; ++i) {
            std::string line;
            std::getline(in, line);
            std::istringstream ss(line);
            Transaction t;
            std::string amt, bal;
            std::getline(ss, t.type,      '|');
            std::getline(ss, amt,          '|');
            std::getline(ss, bal,          '|');
            std::getline(ss, t.timestamp);
            t.amount       = std::stod(amt);
            t.balanceAfter = std::stod(bal);
            history.push_back(t);
        }
    }

protected:
    void recordTransaction(const std::string& type, double amount) {
        history.push_back({type, amount, balance, utils::currentTimestamp()});
    }
};

// ─────────────────────────────────────────────
//  Derived: SavingsAccount  (interest capable)
// ─────────────────────────────────────────────
class SavingsAccount : public Account {
    double interestRate;   // annual %

public:
    SavingsAccount() : Account(), interestRate(4.0) {}

    SavingsAccount(long accNo, const std::string& name,
                   const std::string& pin, double initialDeposit)
        : Account(accNo, name, pin, initialDeposit, "SAVINGS"),
          interestRate(4.0) {}

    double getInterestRate() const { return interestRate; }

    void applyInterest() {
        double interest = balance * interestRate / 100.0;
        balance += interest;
        recordTransaction("INTEREST", interest);
        std::cout << "  Interest applied: ₹" << std::fixed
                  << std::setprecision(2) << interest << "\n";
    }

    void displayDetails() const override {
        Account::displayDetails();
        std::cout << "  Interest Rate : " << interestRate << "% p.a.\n";
    }

    void serialize(std::ofstream& out) const override {
        out << accountNumber << "|" << holderName << "|" << pin << "|"
            << std::fixed << std::setprecision(2) << balance << "|"
            << accountType << "|" << history.size() << "|" << interestRate << "\n";
        for (const auto& t : history)
            out << t.type << "|" << t.amount << "|"
                << t.balanceAfter << "|" << t.timestamp << "\n";
    }
};

// ─────────────────────────────────────────────
//  Derived: CurrentAccount  (overdraft capable)
// ─────────────────────────────────────────────
class CurrentAccount : public Account {
    double overdraftLimit;

public:
    CurrentAccount() : Account(), overdraftLimit(10000.0) {}

    CurrentAccount(long accNo, const std::string& name,
                   const std::string& pin, double initialDeposit)
        : Account(accNo, name, pin, initialDeposit, "CURRENT"),
          overdraftLimit(10000.0) {}

    bool withdraw(double amount) override {
        if (amount <= 0) return false;
        if (balance - amount < -overdraftLimit) return false;   // beyond overdraft
        balance -= amount;
        recordTransaction("WITHDRAW", amount);
        return true;
    }

    void displayDetails() const override {
        Account::displayDetails();
        std::cout << "  Overdraft Limit : ₹"
                  << std::fixed << std::setprecision(2) << overdraftLimit << "\n";
    }
};

// ─────────────────────────────────────────────
//  Bank  – manages all accounts + persistence
// ─────────────────────────────────────────────
class Bank {
    std::string            dataFile {"bank_data.txt"};
    std::string            logFile  {"bank_log.txt"};
    std::vector<Account*>  accounts;
    long                   nextAccountNumber {100001};

    // ── helpers ──────────────────────────────
    Account* findAccount(long accNo) const {
        for (auto* a : accounts)
            if (a->getAccountNumber() == accNo) return a;
        return nullptr;
    }

    void writeLog(const std::string& entry) const {
        std::ofstream log(logFile, std::ios::app);
        if (log) log << utils::currentTimestamp() << " | " << entry << "\n";
    }

public:
    Bank() { loadData(); }

    ~Bank() {
        saveData();
        for (auto* a : accounts) delete a;
    }

    // ─── Account creation ────────────────────
    void createAccount() {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string name, pin, typeChoice;
        double initialDeposit;

        std::cout << "\n  Account Type:\n";
        std::cout << "  1. Savings\n";
        std::cout << "  2. Current\n";
        std::cout << "  Choice: ";
        std::getline(std::cin, typeChoice);

        std::cout << "  Full Name      : ";
        std::getline(std::cin, name);
        if (name.empty()) { std::cout << "  Name cannot be empty.\n"; return; }

        std::cout << "  Set 4-digit PIN: ";
        std::getline(std::cin, pin);
        if (pin.length() != 4 || pin.find_first_not_of("0123456789") != std::string::npos) {
            std::cout << "  Invalid PIN. Must be 4 digits.\n";
            return;
        }

        std::cout << "  Initial Deposit (₹): ";
        std::cin >> initialDeposit;
        if (initialDeposit < 500) {
            std::cout << "  Minimum initial deposit is ₹500.\n";
            return;
        }

        Account* acc = nullptr;
        if (typeChoice == "2")
            acc = new CurrentAccount(nextAccountNumber, name, pin, initialDeposit);
        else
            acc = new SavingsAccount(nextAccountNumber, name, pin, initialDeposit);

        accounts.push_back(acc);
        writeLog("ACCOUNT_OPENED | AccNo=" + std::to_string(nextAccountNumber)
                 + " | Name=" + name);

        std::cout << "\n  ✔ Account created successfully!\n";
        std::cout << "  Your Account Number: " << nextAccountNumber << "\n";
        ++nextAccountNumber;
    }

    // ─── Authenticate + return account ───────
    Account* authenticate() {
        long accNo; std::string pin;
        std::cout << "  Account Number : ";
        std::cin >> accNo;
        std::cout << "  PIN            : ";
        std::cin >> pin;

        Account* acc = findAccount(accNo);
        if (!acc || !acc->verifyPin(pin)) {
            std::cout << "\n  ✘ Invalid account number or PIN.\n";
            writeLog("AUTH_FAILED | AccNo=" + std::to_string(accNo));
            return nullptr;
        }
        writeLog("AUTH_OK | AccNo=" + std::to_string(accNo));
        return acc;
    }

    // ─── Deposit ─────────────────────────────
    void deposit() {
        std::cout << "\n  ── Deposit ──────────────────\n";
        Account* acc = authenticate();
        if (!acc) return;

        double amount;
        std::cout << "  Amount to Deposit (₹): ";
        std::cin >> amount;

        if (acc->deposit(amount)) {
            std::cout << "  ✔ Deposited ₹" << std::fixed << std::setprecision(2)
                      << amount << " | New Balance: ₹" << acc->getBalance() << "\n";
            writeLog("DEPOSIT | AccNo=" + std::to_string(acc->getAccountNumber())
                     + " | Amt=" + std::to_string(amount));
        } else {
            std::cout << "  ✘ Deposit failed. Enter a positive amount.\n";
        }
    }

    // ─── Withdrawal ──────────────────────────
    void withdraw() {
        std::cout << "\n  ── Withdrawal ───────────────\n";
        Account* acc = authenticate();
        if (!acc) return;

        double amount;
        std::cout << "  Amount to Withdraw (₹): ";
        std::cin >> amount;

        if (acc->withdraw(amount)) {
            std::cout << "  ✔ Withdrew ₹" << std::fixed << std::setprecision(2)
                      << amount << " | New Balance: ₹" << acc->getBalance() << "\n";
            writeLog("WITHDRAW | AccNo=" + std::to_string(acc->getAccountNumber())
                     + " | Amt=" + std::to_string(amount));
        } else {
            std::cout << "  ✘ Withdrawal failed. Insufficient funds or invalid amount.\n";
        }
    }

    // ─── Balance enquiry ─────────────────────
    void checkBalance() {
        std::cout << "\n  ── Balance Enquiry ──────────\n";
        Account* acc = authenticate();
        if (!acc) return;
        acc->displayDetails();
    }

    // ─── Mini statement ──────────────────────
    void miniStatement() {
        std::cout << "\n  ── Mini Statement ───────────\n";
        Account* acc = authenticate();
        if (!acc) return;
        acc->displayHistory();
    }

    // ─── Apply interest (savings only) ───────
    void applyInterest() {
        std::cout << "\n  ── Apply Interest ───────────\n";
        Account* acc = authenticate();
        if (!acc) return;
        auto* sav = dynamic_cast<SavingsAccount*>(acc);
        if (!sav) {
            std::cout << "  Interest is only applicable for Savings accounts.\n";
            return;
        }
        sav->applyInterest();
        std::cout << "  New Balance: ₹" << std::fixed << std::setprecision(2)
                  << acc->getBalance() << "\n";
        writeLog("INTEREST | AccNo=" + std::to_string(acc->getAccountNumber()));
    }

    // ─── Close account ────────────────────────
    void closeAccount() {
        std::cout << "\n  ── Close Account ────────────\n";
        Account* acc = authenticate();
        if (!acc) return;

        if (!utils::confirm("Are you sure you want to close this account?")) return;

        long accNo = acc->getAccountNumber();
        accounts.erase(std::remove(accounts.begin(), accounts.end(), acc),
                       accounts.end());
        delete acc;
        writeLog("ACCOUNT_CLOSED | AccNo=" + std::to_string(accNo));
        std::cout << "  ✔ Account " << accNo << " closed successfully.\n";
    }

    // ─── Persistence: save ────────────────────
    void saveData() const {
        std::ofstream out(dataFile);
        if (!out) { std::cerr << "  Error: cannot write " << dataFile << "\n"; return; }

        out << nextAccountNumber << "\n";
        out << accounts.size()   << "\n";

        for (const auto* a : accounts)
            a->serialize(out);
    }

    // ─── Persistence: load ────────────────────
    void loadData() {
        std::ifstream in(dataFile);
        if (!in) return;   // first run — no file yet

        int total;
        in >> nextAccountNumber >> total;
        in.ignore();

        for (int i = 0; i < total; ++i) {
            std::string line;
            std::getline(in, line);
            std::istringstream ss(line);

            long        accNo; double bal; int txCount;
            std::string name, pin, type, sAccNo, sBal, sTxCount, extra;

            std::getline(ss, sAccNo,   '|');
            std::getline(ss, name,     '|');
            std::getline(ss, pin,      '|');
            std::getline(ss, sBal,     '|');
            std::getline(ss, type,     '|');
            std::getline(ss, sTxCount, '|');
            std::getline(ss, extra);      // interestRate or empty

            accNo   = std::stol(sAccNo);
            bal     = std::stod(sBal);
            txCount = std::stoi(sTxCount);

            Account* acc = nullptr;
            if (type == "SAVINGS") {
                auto* sav   = new SavingsAccount();
                acc = sav;
            } else {
                acc = new CurrentAccount();
            }

            // Directly write fields via a friend-style trick:
            // (or expose a load constructor — we use a protected setter approach)
            // Here we reconstruct via re-assignment using placement:
            //   We'll use a simple load constructor pattern:
            acc->loadFields(accNo, name, pin, bal, type);

            acc->deserialize(in, txCount);
            accounts.push_back(acc);
        }
    }
};

// ─────────────────────────────────────────────
//  Main menu
// ─────────────────────────────────────────────
void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════╗\n";
    std::cout << "  ║      SECURE BANK MANAGEMENT SYSTEM       ║\n";
    std::cout << "  ║         Powered by C++ & OOP             ║\n";
    std::cout << "  ╚══════════════════════════════════════════╝\n\n";
}

void printMenu() {
    std::cout << "\n  ┌──────────────────────────────┐\n";
    std::cout << "  │           MAIN MENU          │\n";
    std::cout << "  ├──────────────────────────────┤\n";
    std::cout << "  │  1. Create New Account       │\n";
    std::cout << "  │  2. Deposit Money            │\n";
    std::cout << "  │  3. Withdraw Money           │\n";
    std::cout << "  │  4. Check Balance            │\n";
    std::cout << "  │  5. Mini Statement           │\n";
    std::cout << "  │  6. Apply Interest (Savings) │\n";
    std::cout << "  │  7. Close Account            │\n";
    std::cout << "  │  0. Exit                     │\n";
    std::cout << "  └──────────────────────────────┘\n";
    std::cout << "  Choice: ";
}

int main() {
    Bank bank;
    int choice;

    printBanner();

    while (true) {
        printMenu();
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
            case 1: bank.createAccount();  break;
            case 2: bank.deposit();        break;
            case 3: bank.withdraw();       break;
            case 4: bank.checkBalance();   break;
            case 5: bank.miniStatement();  break;
            case 6: bank.applyInterest();  break;
            case 7: bank.closeAccount();   break;
            case 0:
                std::cout << "\n  Data saved. Goodbye!\n\n";
                return 0;
            default:
                std::cout << "  Invalid choice. Try again.\n";
        }
        utils::pause();
    }
}
