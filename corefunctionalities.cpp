#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <atomic>
#include <chrono>
#include <random>
#include<vector>
#include<mutex>
using namespace std;
mutex mtx;
atomic<int> global_process_id(1);
struct Process {
    int process_id;
    int account_id;
    string operation;
    double amount;
    string status;
};
vector<Process> process_table;

class Account {
public:
    int account_id;
    double balance;

    Account(int id, double initial_balance) : account_id(id), balance(initial_balance) {}

    void deposit(double amount) {
        balance += amount;
    }

    void withdraw(double amount) {
        if (balance >= amount) {
            balance -= amount;
        }
        else {
            throw  runtime_error("Insufficient balance!");
        }
    }

    double get_balance() const {
        return balance;
    }
};

class BankingSystem {
private:
    vector<Account> accounts;

    int generate_random_id() {
        static  mt19937 generator(random_device{}());
        static  uniform_int_distribution<int> distribution(1000, 9999);
        return distribution(generator);
    }

public:
    void create_account(double initial_balance) {
        int account_id = generate_random_id();


        while (find_account_or_null(account_id) != nullptr) {
            account_id = generate_random_id();
        }

        accounts.push_back(Account(account_id, initial_balance));
        cout << "Account " << account_id << " created with initial balance: " << initial_balance << endl;
    }

    Account* find_account_or_null(int account_id) {
        for (auto& account : accounts) {
            if (account.account_id == account_id) {
                return &account;
            }
        }
        return nullptr;
    }

    Account* find_account(int account_id) {
        Account* account = find_account_or_null(account_id);
        if (!account) {
            throw  runtime_error("Account not found!");
        }
        return account;
    }

    void deposit(int account_id, double amount) {
        Account* account = find_account(account_id);
        account->deposit(amount);
        cout << "Deposited " << amount << " to account " << account_id << ". New balance: " << account->get_balance() << endl;
    }

    void withdraw(int account_id, double amount) {
        Account* account = find_account(account_id);
        account->withdraw(amount);
        cout << "Withdrew " << amount << " from account " << account_id << ". New balance: " << account->get_balance() << endl;
    }

    void check_balance(int account_id) {
        Account* account = find_account(account_id);
        cout << "Balance for account " << account_id << ": " << account->get_balance() << endl;
    }
};

void display_process_table() {
    cout << "\n--- Process Table ---\n";
    cout << "PID\tAccount ID\tOperation\tAmount\tStatus\n";
    for (const auto& process : process_table) {
        cout << process.process_id << "\t" << process.account_id << "\t\t" << process.operation << "\t"
            << process.amount << "\t" << process.status << endl;
    }
    cout << "---------------------\n";
}

void process_transaction(BankingSystem& bank, int account_id, string operation, double amount) {
    int process_id = global_process_id++;
    {

        lock_guard< mutex> lock(mtx);
        string amount_str = (operation == "check_balance") ? "N/A" : to_string(amount);
        process_table.push_back({ process_id, account_id, operation, amount, "Running" });
        display_process_table();
    }

    try {
        if (operation == "deposit") {
            bank.deposit(account_id, amount);
        }
        else if (operation == "withdraw") {
            bank.withdraw(account_id, amount);
        }
        else if (operation == "check_balance") {
            bank.check_balance(account_id);
        }


        lock_guard< mutex> lock(mtx);
        for (auto& process : process_table) {
            if (process.process_id == process_id) {
                process.status = "Completed";
            }
        }
    }
    catch (const  exception& e) {
        cerr << "Error in process " << process_id << ": " << e.what() << endl;

        lock_guard< mutex> lock(mtx);
        for (auto& process : process_table) {
            if (process.process_id == process_id) {
                process.status = "Failed";
            }
        }
    }


    {
        lock_guard< mutex> lock(mtx);
        display_process_table();
    }
}


// Menu-Driven Banking System
void menu() {
    BankingSystem bank;
    int choice;

    do {
        cout << "\n--- Banking System Menu ---\n";
        cout << "1. Create Account\n";
        cout << "2. Deposit\n";
        cout << "3. Withdraw\n";
        cout << "4. Check Balance\n";
        cout << "5. Display Process Table\n";
        cout << "6. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        int account_id;
        double amount;
        string operation;

        switch (choice) {
        case 1:
            cout << "Enter Initial Balance: ";
            cin >> amount;
            bank.create_account(amount);
            break;

        case 2:
            cout << "Enter Account ID and Deposit Amount: ";
            cin >> account_id >> amount;
            thread(&process_transaction, ref(bank), account_id, "deposit", amount).detach();
            break;
        case 3:
            cout << "Enter Account ID and Withdrawal Amount: ";
            cin >> account_id >> amount;
            thread(&process_transaction, ref(bank), account_id, "withdraw", amount).detach();
            break;
        case 4:
            cout << "Enter Account ID: ";
            cin >> account_id;
            thread(&process_transaction, ref(bank), account_id, "check_balance", 0).detach();
            break;
        case 5:
            display_process_table();
            break;
        case 6:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid choice! Please try again.\n";
        }
    } while (choice != 6);
}

int main() {
    menu();
    return 0;
}
