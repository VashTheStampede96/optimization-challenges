#include <bits/stdc++.h>
#include "utils.cpp"    // ScoreManager, Rand
using namespace std;
constexpr array<const char*, 5> ValidNames = {{"a", "b", "c", "d", "e"}};
string ProblemName;
uint64_t SEED = 0;
size_t CANDIDATE_MOVES = 1;
double TIME_LIMIT = oo;

bool MAXIMIZE = true;


using ScoreType = int64_t; // CHANGE IF NEEDED
using SolutionType = vector<bool>; // Vector of 0/1 if ingredient is present in the solution
using MoveType = int; // A move inserts/deletes an ingredient from the pizza




void ArgSanitize(int, char**);
void ReadInput(string);
void ReadSolution(string);
void DoMagic();
ScoreType GetScore(const SolutionType&);
MoveType DrawRandomMove(PRNG&);
ScoreType DeltaCost(const SolutionType&, const MoveType&);
bool IsBetterMove(ScoreType, ScoreType); // SIMULATED ANNEALING
void ApplyMove(SolutionType&, const MoveType&);
void PrintSolution(string);

double GlobalTemperature = 1.0, CoolingFactor = 0.99999;
random_device rd;
default_random_engine eng(rd());
uniform_real_distribution<double> prob(0.0, 1.0);


int C, K;
map<string, int> ingr_to_id;
vector<string> id_to_ingr;

struct Client {
    int L, D;
    vector<int> likes, dislikes;

    void read(ifstream & cin) {

        cin >> L;
        for(int i=0; i<L; ++i) {
            static string ingr_name;
            cin >> ingr_name;
            if(ingr_to_id.find(ingr_name) == ingr_to_id.end()) {
                cerr << "New ingredient " << ingr_name << "!\n";
                ingr_to_id[ingr_name] = id_to_ingr.size();
                id_to_ingr.emplace_back(ingr_name);
            }

            likes.emplace_back(ingr_to_id[ingr_name]);
        }

        cin >> D;
        for(int i=0; i<D; ++i) {
            static string ingr_name;
            cin >> ingr_name;
            if(ingr_to_id.find(ingr_name) == ingr_to_id.end()) {
                cerr << "New ingredient " << ingr_name << "!\n";
                ingr_to_id[ingr_name] = id_to_ingr.size();
                id_to_ingr.emplace_back(ingr_name);
            }

            dislikes.emplace_back(ingr_to_id[ingr_name]);
        }
    }
};


vector<Client> clients;
vector<vector<int>> who_likes, who_dislikes;


SolutionType best, initial;


int main(int argc, char** argv)
{
    ios_base::sync_with_stdio(false);
    ArgSanitize(argc, argv);

    ReadInput( string("input/") + ProblemName + ".txt" );
    ReadSolution( string("output/") + ProblemName + ".greedy" );
    DoMagic();
    PrintSolution( string("output/") + ProblemName + ".opt" );

    return 0;
}


void ArgSanitize(int argc, char** argv)
{
    for(int i=1; i<argc; ) {
        string arg = argv[i];
        if(arg.substr(0, 2) == "--") {
            arg = arg.substr(2);
            if(arg == "problem") {
                if(i+1 < argc) {
                    ProblemName = argv[i+1];
                    i += 2;

                    if(find(ValidNames.begin(), ValidNames.end(), ProblemName)
                        == ValidNames.end()
                    ) {
                        cerr << "Problem name is not valid!" << endl;
                        exit(1);
                    }
                }else {
                    cerr << "Missing name after `--problem` argument!" << endl;
                    exit(1);
                }
            }else if(arg == "seed") {
                if(i+1 < argc) {
                    SEED = atoi(argv[i+1]);
                    i += 2;
                }else {
                    cerr << "Missing seed after `--seed` argument!" << endl;
                    exit(1);
                }
            }else if(arg == "time-limit") {
                if(i+1 < argc) {
                    TIME_LIMIT = atoi(argv[i+1]);
                    i += 2;
                }else {
                    cerr << "Missing time-limit after `--time-limit` argument!" << endl;
                    exit(1);
                }
            }else if(arg == "cand-limit") {
                if(i+1 < argc) {
                    CANDIDATE_MOVES = atoi(argv[i+1]);
                    i += 2;
                }else {
                    cerr << "Missing number of candidates after `--cand-limit` argument!" << endl;
                    exit(1);
                }
            }else if(arg == "minimize") {
                MAXIMIZE = false;
                i += 1;
            }else {
                cerr << "Unknown argument --" << arg << ". Valid arguments are: " << endl;
                cerr << "\t--problem <problem name>" << endl;
                cerr << "\t--seed <seed value>" << endl;
                cerr << "\t--time-limit <seconds>" << endl;
                cerr << "\t--cand-limit <number of moves>" << endl;
                cerr << "\t--minimize" << endl;
                exit(1);
            }
        }else {
            cerr << "Wrong form of argument. Must be pre-pended with `--`." << endl;
            exit(1);
        }
    }

    if(CANDIDATE_MOVES == 1) {
        cerr << "Warning! CANDIDATE_MOVES set to 1!" << endl;
    }

    if(SEED == 0) {
        cerr << "Warning! SEED set to 0!" << endl;
    }

    if(TIME_LIMIT == oo) {
        cerr << "Warning! TIME_LIMIT set to infinity!" << endl;
    }

    if(ProblemName == "UNKNOWN") {
        cerr << "Missing problem name. Use `--problem <problem name>` argument!" << endl;
        exit(1);
    }
}

void ReadInput(string filename)
{
    ifstream in(filename);
    if(not in) {
        cerr << "File " << filename << " does not exist!" << endl;
        exit(1);
    }

    cerr << "Reading input from " << filename << " ... " << endl;
    in >> C;
    clients.resize(C);

    for(int i=0; i<C; ++i) {
        auto & client = clients[i];
        client.read(in);
    }

    K = ingr_to_id.size();
    who_likes.resize(K);
    who_dislikes.resize(K);
    for(int i=0; i<C; ++i) {
        auto & client = clients[i];
        for(const auto ingr : client.likes) {
            who_likes[ingr].emplace_back(i);
        }
        for(const auto ingr : client.dislikes) {
            who_dislikes[ingr].emplace_back(i);
        }
    }
    initial.resize(K, false);
}

void ReadSolution(string filename)
{
    ifstream in(filename);
    if(not in) {
        cerr << "File " << filename << " does not exist!" << endl;
        exit(1);
    }

    // Fill "solution" variable
    cerr << "Reading solution from " << filename << " ... " << endl;
    int I;
    in >> I;
    for(int i=0; i<I; ++i) {
        static string ingr;
        in >> ingr;
        initial[ ingr_to_id[ingr] ] = true;
    }
}

void DoMagic()
{
    cerr << "Invoking the power of the underworld..." << endl;

    auto t0 = chrono::high_resolution_clock::now();
    auto t1 = t0;

    SolutionType current = best = initial;
    ScoreType initialScore = GetScore(current);
    ScoreType bestScore = initialScore;
    ScoreManager<ScoreType> scoreManager(initialScore, MAXIMIZE);

    PRNG prng(SEED);

    do {
        // Draw some number of random moves and apply the best one

        vector<ScoreType> delta(CANDIDATE_MOVES, MAXIMIZE ? -oo : +oo);
        vector<MoveType> candidate(CANDIDATE_MOVES);

        #pragma omp parallel for
        for(size_t r=0; r<CANDIDATE_MOVES; ++r) {
            MoveType mv = DrawRandomMove(prng);
            delta[r] = DeltaCost(current, mv);
            candidate[r] = mv;
        }

        ScoreType bestDelta = MAXIMIZE ? -oo : +oo;
        MoveType bestMove = -1; // SET CORRECT DUMMY MOVE
        for(size_t r=0; r<CANDIDATE_MOVES; ++r) {
            if( IsBetterMove(delta[r], bestDelta) ) {
                bestDelta = delta[r];
                bestMove = candidate[r];
            }
        }

        if(bestDelta != (MAXIMIZE ? -oo : +oo)) {
            ApplyMove(current, bestMove);
            scoreManager += bestDelta;
        }

        if(scoreManager.score * (MAXIMIZE ? +1 : -1) > bestScore * (MAXIMIZE ? +1 : -1)) {
            bestScore = scoreManager.score;
	        best = current;
        }


        t1 = chrono::high_resolution_clock::now();

        GlobalTemperature *= CoolingFactor;

    }while( chrono::duration<double>(t1 - t0).count() < TIME_LIMIT );

    cerr << "Optimized score: " << bestScore << "\t\t[Improvement: " << 1. * abs(bestScore - initialScore) / bestScore << "]" << endl;
    cerr << "Global Temperature is " << GlobalTemperature << endl;
}


ScoreType GetScore(const SolutionType & sol)
{
    ScoreType score=0;
    for(int i=0; i<C; ++i) {
        const auto & client = clients[i];
        bool is_sat = true;
        for(size_t j=0; is_sat and j<client.dislikes.size(); ++j) {
            size_t ingr = client.dislikes[j];
            is_sat = is_sat and (not sol[ingr]);
        }
        for(size_t j=0; is_sat and j<client.likes.size(); ++j) {
            size_t ingr = client.likes[j];
            is_sat = is_sat and sol[ingr];
        }

        score += is_sat;
    }

    return score;
}

ScoreType DeltaCost(const SolutionType & sol, const MoveType & mv)
{
    ScoreType delta=0;
    SolutionType tmp(sol);
    ApplyMove(tmp, mv);
    delta = GetScore(tmp) - GetScore(sol);

    return delta;
}

// SIMULATED ANNEALING
bool IsBetterMove(ScoreType newDelta, ScoreType oldDelta)
{
    if( newDelta * (MAXIMIZE ? -1 : +1) > 0.0 ) {
        double p = exp((MAXIMIZE ? +1 : -1) * newDelta / GlobalTemperature);
        return p > prob(eng);
    }

    return true;
}

MoveType DrawRandomMove(PRNG & Random)
{
    return Random() % K;
}

void ApplyMove(SolutionType & sol, const MoveType & mv)
{
    sol[mv] = sol[mv] ? false : true;
}


void PrintSolution(string filename)
{
    ofstream out(filename);
    
    cerr << "Writing solution to file " << filename << " ... " << endl;

    // Print "best"
    int M = count_if(best.begin(), best.end(), [](bool x) { return x; });
    out << M << " ";
    for(int i=0; i<K; ++i) {
        if(best[i]) {
            out << id_to_ingr[i] << " ";
        }
    }
    out << endl;
}