// Fedchuk Sofiia k-26, variant 4

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>
#include <mutex>
#include <random>
#include <iomanip>

using namespace std;
using namespace chrono;

class A {
    static constexpr int M = 3;
    int fields[M] = { 1, 1, 1 };
    mutable mutex mtx[2];
    static constexpr int lockOf[M] = { 0, 1, 0 };  

public:
    int get(int i) const {
        lock_guard<mutex> lk(mtx[lockOf[i]]);
        return fields[i];
    }

    void set(int i, int v) {
        lock_guard<mutex> lk(mtx[lockOf[i]]);
        fields[i] = v;
    }

    operator string() const {
        lock_guard<mutex> lk0(mtx[0]);   
        lock_guard<mutex> lk1(mtx[1]);
        return to_string(fields[0]) + ", " + to_string(fields[1]) + ", " + to_string(fields[2]);
    }
};

struct Cmd {
    char type;  
    int field;
    int value;
};

void generateFile(const string& filename, const vector<double>& w, int numCommands, unsigned seed) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create file " << filename << endl;
        return;
    }
    mt19937 gen(seed);
    discrete_distribution<> d(w.begin(), w.end());
    for (int i = 0; i < numCommands; i++) {
        int k = d(gen);
        if (k == 6) file << "string\n";
        else if (k % 2 == 0) file << "read " << k / 2 << "\n";
        else file << "write " << k / 2 << " 1\n";
    }
}

vector<Cmd> loadFile(const string& filename) {
    vector<Cmd> cmds;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        return cmds;
    }
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string c;
        ss >> c;
        Cmd cmd{ 's', 0, 0 };
        if (c == "write") { cmd.type = 'w'; ss >> cmd.field >> cmd.value; }
        else if (c == "read") { cmd.type = 'r'; ss >> cmd.field; }
        cmds.push_back(cmd);
    }
    return cmds;
}

void run(const vector<Cmd>& cmds, A& obj, long long& sink) {
    long long local = 0;
    for (const Cmd& c : cmds) {
        if (c.type == 'w') obj.set(c.field, c.value);
        else if (c.type == 'r') local += obj.get(c.field);
        else local += static_cast<string>(obj).size();
    }
    sink = local;   
}

double measure(const vector<vector<Cmd>>& perThread) {
    A obj;
    vector<long long> sinks(perThread.size(), 0);
    vector<thread> threads;
    auto start = high_resolution_clock::now();
    if (perThread.size() == 1) {
        run(perThread[0], obj, sinks[0]);
    } else {
        for (size_t i = 0; i < perThread.size(); i++)
            threads.emplace_back(run, cref(perThread[i]), ref(obj), ref(sinks[i]));
        for (auto& t : threads) t.join();
    }
    auto end = high_resolution_clock::now();
    long long total = 0;
    for (auto s : sinks) total += s;
    if (total == -1) cout << "";   
    return duration_cast<microseconds>(end - start).count() / 1000.0;
}

int main() {
    const int TOTAL = 1200000;   
    const int REPEATS = 5;

    struct Scenario { string name; vector<double> w; };
    vector<Scenario> scenarios = {
        { "a) variant 4 frequencies", { 15, 1, 50, 3, 5, 1, 25 } },
        { "b) equal frequencies",     { 1, 1, 1, 1, 1, 1, 1 } },
        { "c) custom (mismatching)",  { 30, 25, 1, 1, 20, 20, 3 } },
    };

    cout << "Generating test files..." << endl;
    for (int s = 0; s < 3; s++)
        for (int th = 1; th <= 3; th++)
            for (int t = 0; t < th; t++)
                generateFile("input" + to_string(s + 1) + to_string(th) + "_" + to_string(t) + ".txt",
                             scenarios[s].w, TOTAL / th, 1000 * s + 10 * th + t + 1);
    cout << "File generation complete." << endl;

    double table[3][3];
    for (int s = 0; s < 3; s++) {
        cout << "\n" << scenarios[s].name << endl;
        for (int th = 1; th <= 3; th++) {
            vector<vector<Cmd>> data;
            for (int t = 0; t < th; t++)
                data.push_back(loadFile("input" + to_string(s + 1) + to_string(th) + "_" + to_string(t) + ".txt"));

            double sum = 0;
            for (int r = 0; r < REPEATS; r++) {
                double ms = measure(data);
                cout << "  " << th << " thread(s), run " << r + 1 << ": " << fixed << setprecision(2) << ms << " ms" << endl;
                sum += ms;
            }
            table[s][th - 1] = sum / REPEATS;
        }
    }

    cout << "\nAverage time (ms), " << TOTAL << " operations, " << REPEATS << " runs:" << endl;
    cout << left << setw(28) << "" << setw(12) << "1 thread" << setw(12) << "2 threads" << setw(12) << "3 threads" << endl;
    for (int s = 0; s < 3; s++) {
        cout << left << setw(28) << scenarios[s].name;
        for (int t = 0; t < 3; t++) cout << setw(12) << fixed << setprecision(2) << table[s][t];
        cout << endl;
    }
    return 0;
}