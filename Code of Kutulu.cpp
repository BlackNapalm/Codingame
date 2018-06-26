#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <math.h>

using namespace std;

//enum State {SPAWNING = 0, WANDERING};
const char WALL = '#', SPAWN = 'w', SHELTER = 'U';
const double DEFDANGER = 9999;
vector<int> yelledat;

class Tile {
    private:
        int x, y;
        char tiletype;
        double baseDanger = DEFDANGER, turnDanger = baseDanger;
    public:
        Tile () {}
        Tile (int x, int y, char t) : x(x), y(y), tiletype(t) {}
        int get_x() {return x;}
        int get_y() {return y;}
        char get_type() {return tiletype;}
        double get_baseDanger() {return baseDanger;}
        double get_turnDanger() {return turnDanger;}
        void set_baseDanger(double d) {baseDanger = d;}
        void set_turnDanger(double d) {turnDanger = d;}
        void adj_turnDanger(double d) {turnDanger += d;}
        int get_distance(Tile other) {
            return abs(this->x - other.x) + abs(this->y - other.y);
        }
};
class Unit {
    protected:
        int x, y, id;
    public:
        Unit (int c, int r, int i) : x(c), y(r), id(i) {}
        int get_distance(Unit other) {
            return abs(this->x - other.x) + abs(this->y - other.y);
        }
        int get_distance(Tile other) {
            return abs(this->x - other.get_x()) + abs(this->y - other.get_y());
        }
        int get_x() {return x;}
        int get_y() {return y;}
        int get_id() {return id;}
        bool same(Unit other) {
            return this->id == other.id;
        }
        bool samepos(Unit other) {
            if (x == other.get_x() and y == other.get_y()) {
                return true;
            } else {return false;}
        }
};
class Effect: public Unit {
    private:
        int turns, trigger_id, target_id;
    public:
        Effect (int c, int r, int i, int t, int sid, int tid) : Unit(c, r, i), turns(t), trigger_id(sid), target_id(tid) {}
};
class Shelter: public Unit {
    private:
        int energy;
    public:
        Shelter (int c, int r, int i, int e) : Unit(c, r, i), energy(e) {}
        int get_energy() {return energy;}
};
class Plan: public Unit {
    private:
        int turns;
    public:
        Plan (int c, int r, int i, int e) : Unit(c, r, i), turns(e) {}
        int get_turns() {return turns;}
};
class Wanderer: public Unit {
    private:
        int turns, state, target;
    public:
        Wanderer (int x, int y, int id, int turn, int state, int target) : Unit(x, y, id), turns(turn), state(state), target(target) {}
        int get_state() {return state;}
        int get_target() {return target;}
        int get_turns() {return turns;}
};
class Explorer: public Unit {
    private:
        int sanity, plans, lights;
    public:
        Explorer (int x, int y, int id, int san, int plan, int light) : Unit(x, y, id), sanity(san), plans(plan), lights(light) {}
        int get_sanity() {return sanity;}
        bool yell_check(vector<Explorer> e, vector<Wanderer> w) {
            bool canyell = false;
            for (Wanderer y : w) {
                if (get_distance(y) == 1) {
                    return false;
                }
            }
            for (Explorer x : e) {
                if (same(x)) {continue;}
                bool yellat = false;
                for (int i : yelledat) {
                    if (x.get_id() == i) {
                        yellat = true;
                    }
                }
                if (yellat) {continue;}
                if (get_distance(x) > 1) {continue;}
                bool closewanderer = false;
                for (Wanderer y : w) {
                    if (y.get_state() == 0) {continue;}
                    if (x.get_distance(y) > 2) {continue;}
                        closewanderer = true;
                }
                if (!closewanderer) {return false;}
                if (e.back().get_id() == x.get_id()) {
                    canyell = true;
                }
            }
            return canyell;
        }
        bool light_check(vector<Explorer> e, vector<Wanderer> w) {
            if (lights < 1) {return false;}
            for (Explorer x : e) {
                if (same(x)) {continue;}
                if (samepos(x)) {continue;}
                for (Wanderer y : w) {
                    if (get_distance(y) == 1) {return false;}
                    if (y.get_target() != id) {continue;}
                    if (get_distance(x) < x.get_distance(y)) {continue;}
                    if (get_distance(x) > 6) {continue;}
                    if (get_distance(y) + 4 > x.get_distance(y)) {
                        return true;
                    }
                }
            }
        }
        bool plan_check(vector<Explorer> e, vector<Wanderer> w) {
            if (plans < 1) {return false;}
            if (sanity > 250 - e.size() * 15) {return false;}
            for (Wanderer x : w) {
                if (get_distance(x) == 1) {return false;}
            }
            int ecount = 0;
            for (Explorer x : e) {
                if (get_distance(x) < 3 ) {ecount++;}
            }
            if (ecount == e.size()) {return true;}
            return false;
        }
};
class Slasher: public Unit {
    private:
        int turns, state, target;
    public:
        Slasher (int x, int y, int id, int turn, int state, int target) : Unit(x, y, id), turns(turn), state(state), target(target) {}
        int get_turns() {return turns;}
        int get_state() {return state;}
};
class GameBoard {
    private:
        int width, height, sanityLossLonely, sanityLossGroup, wandererSpawnTime, wandererLifeTime;
        vector<vector<Tile>> board;
        vector<Tile> spawns;
        vector<Tile> shelters;
    public:
        GameBoard(int w, int h) : width(w), height(h) {}
        void set_BaseDanger() {
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    char type = board[j][i].get_type();
                    if (type == WALL) {continue;}
                    double danger = sanityLossLonely;
                    for (Tile t : spawns) {
                        int dist = t.get_distance(board[j][i]);
                        danger += (width * height - dist) / (width + height);
                        board[j][i].set_baseDanger(danger);
                    }
                }
            }
        }
        void set_Board(vector<string> map) {
            for (int i = 0; i < width; i++) {
                vector<Tile> line;
                for (int j = 0; j < height; j++) {
                    Tile foo(i, j, map[j][i]);
                    line.push_back(foo);
                    if (foo.get_type() == SPAWN) {
                        spawns.push_back(foo);
                    } else if (foo.get_type() == SHELTER) {
                        shelters.push_back(foo);
                    }
                }
                board.push_back(line);
            }
        }
        void set_BoardParam(int sll, int slg, int wst, int wlt) {
            sanityLossLonely = sll;
            sanityLossGroup = slg;
            wandererSpawnTime = wst;
            wandererLifeTime = wlt;
            this->set_BaseDanger();
        }
        void print_Board() {
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    cerr << board[j][i].get_type() << "(" << board[j][i].get_turnDanger() << ")";
                }
                cerr << endl;
            }
        }
        vector<Tile> get_safeTiles(Explorer e) {
            vector<Tile> safeTiles;
            double lowDanger = DEFDANGER;
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    double danger = board[j][i].get_turnDanger() + (e.get_distance(board[j][i]) / 7); // sqrt(width + height));
                    if (danger < lowDanger) {
                        lowDanger = danger;
                        safeTiles.clear();
                    }
                    if (danger == lowDanger) {
                        safeTiles.push_back(board[j][i]);
                    }
                }
            }
            return safeTiles;
        }
        string pathfind (Explorer e, Tile t) {
            if (e.get_x() == t.get_x() and e.get_y() == t.get_y()) {return "WAIT";} //return "WAIT"
            vector<tuple<Tile, string, double>> myqueue;
            tuple<Tile, string, double> foo (board[e.get_x()][e.get_y()], "", board[e.get_x()][e.get_y()].get_turnDanger());
            myqueue.push_back(foo);
            vector<Tile> checkedTiles;
            while (!myqueue.empty()) {
                int queuepos = 0, minDanger = get<2>(myqueue.front());
                for (int i = 0; i < myqueue.size(); i++){
                    if (get<2>(myqueue[i]) < minDanger) {
                        queuepos = i;
                        minDanger = get<2>(myqueue[i]);
                    }
                }
                Tile check;
                for (int i = -1; i < 2; i++) {
                    for (int j = -1; j < 2; j++) {
                        if (abs(i) + abs(j) != 1) {continue;}
                        check = board[get<0>(myqueue[queuepos]).get_x() + i][get<0>(myqueue[queuepos]).get_y() + j];
                        if (check.get_type() != WALL) {
                            bool checked = false;
                            for (Tile t : checkedTiles) {
                                if (t.get_x() == check.get_x() and t.get_y() == check.get_y()) {checked = true;}
                            }
                            if (!checked) {
                                string dir = "";
                                if (get<1>(myqueue[queuepos]) == "") {
                                    if (i == -1) {
                                        dir = "LEFT";
                                    } else if (i == 1) {
                                        dir = "RIGHT";
                                    } else if (j == -1) {
                                        dir = "UP";
                                    } else if (j == 1) {
                                        dir = "DOWN";
                                    }
                                } else {dir = get<1>(myqueue[queuepos]);}
                                if (check.get_x() == t.get_x() and check.get_y() == t.get_y()) {return dir;}
                                get<0>(foo) = check;
                                get<1>(foo) = dir;
                                get<2>(foo) = check.get_turnDanger() / e.get_distance(check) + get<2>(myqueue[queuepos]);
                                myqueue.push_back(foo);
                                checkedTiles.push_back(check);
                            }
                        }
                    }
                }
                myqueue.erase(myqueue.begin());
            }
            return "WAIT";
        }
        string get_movement(Explorer e) {
            vector<Tile> safeTiles;
            safeTiles = get_safeTiles(e);
            Tile safeTile;
            int lowDist = width + height;
            for (Tile t : safeTiles) {
                int dist = e.get_distance(t);
                if (dist < lowDist) {
                    safeTile = t;
                    lowDist = dist;
                }
            }
            return pathfind(e, safeTile);
        }
        void adjDanger(Unit u, int dist, double adjust) {
            for (int i = -dist; i < dist + 1; i++) {
                for (int j = -dist; j < dist + 1; j++) {
                    if (abs(i) + abs(j) > dist) {continue;}
                    if (u.get_x() + i < 0 or u.get_x() + i >= width) {continue;}
                    if (u.get_y() + j < 0 or u.get_y() + j >= height) {continue;}
                    board[u.get_x() + i][u.get_y() + j].adj_turnDanger(adjust);
                }
            }
        }
        void adjDanger(Unit u, double adjust) {
            board[u.get_x()][u.get_y()].adj_turnDanger(adjust);
            bool up = true, down = true, left = true, right = true;
            for (int i = 1; i < max(width, height); i++) {
                if (u.get_x() - i >= 0 and left) {
                    if (board[u.get_x() - i][u.get_y()].get_type() != WALL) {
                        board[u.get_x() - i][u.get_y()].adj_turnDanger(adjust);
                    } else {left = false;}
                }
                if (u.get_x() + i < width and right) {
                    if (board[u.get_x() + i][u.get_y()].get_type() != WALL) {
                        board[u.get_x() + i][u.get_y()].adj_turnDanger(adjust);
                    } else {right = false;}
                }
                if (u.get_y() - i >= 0 and up) {
                    if (board[u.get_x()][u.get_y() - i].get_type() != WALL) {
                        board[u.get_x()][u.get_y() - i].adj_turnDanger(adjust);
                    } else {up = false;}
                }
                if (u.get_y() + i < height and down) {
                    if (board[u.get_x()][u.get_y() + i].get_type() != WALL) {
                        board[u.get_x()][u.get_y() + i].adj_turnDanger(adjust);
                    } else {down = false;}
                }
            }
        }
        void set_turnDanger(vector<Explorer> e, vector<Wanderer> w, vector<Slasher> s, vector<Shelter> u, vector<Plan> p) {
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    board[j][i].set_turnDanger(board[j][i].get_baseDanger());
                }
            }
            for (Explorer x : e) {
                if (x.same(e.front())) {continue;}
                double danAdj = sanityLossGroup - sanityLossLonely;
                adjDanger(x, 2, danAdj/2);
                adjDanger(x, 1, danAdj/2);
            }
            for (Wanderer x : w) {
                double danAdj = 20;//x.get_turns();
                //for (int i = x.get_turns(); i > 0; i--) {
                    
                //}
                adjDanger(x, 2, danAdj);
            }
            for (Slasher x : s) {
                double danAdj = 10;
                if (x.get_state() == 0 or x.get_state() == 4) {danAdj *= (6-x.get_turns())/6;}
                adjDanger(x, 2, danAdj);
                adjDanger(x, danAdj);
            }
            for (Shelter x : u) {
                double danAdj = -5 + -5 * (x.get_energy()/(e.front().get_distance(x)+1));
                adjDanger(x, 0, danAdj);
            }
            for (Plan x : p) {
                double danAdj = -3 * x.get_turns();
                adjDanger(x, 2, danAdj);
            }
        }
};

int main()
{
    int width;
    cin >> width; cin.ignore();
    int height;
    cin >> height; cin.ignore();
    GameBoard board (width, height);
    vector<string> map;
    for (int i = 0; i < height; i++) {
        string line;
        getline(cin, line);
        map.push_back(line);
    }
    int sanityLossLonely; // how much sanity you lose every turn when alone, always 3 until wood 1
    int sanityLossGroup; // how much sanity you lose every turn when near another player, always 1 until wood 1
    int wandererSpawnTime; // how many turns the wanderer take to spawn, always 3 until wood 1
    int wandererLifeTime; // how many turns the wanderer is on map after spawning, always 40 until wood 1
    cin >> sanityLossLonely >> sanityLossGroup >> wandererSpawnTime >> wandererLifeTime; cin.ignore();
    board.set_Board(map);
    board.set_BoardParam(sanityLossLonely, sanityLossGroup, wandererSpawnTime, wandererLifeTime);
    // game loop
    while (1) {
        int entityCount; // the first given entity corresponds to your explorer
        cin >> entityCount; cin.ignore();
        vector<Explorer> explorers;
        vector<Wanderer> wanderers;
        vector<Slasher> slashers;
        vector<Shelter> shelters;
        vector<Plan> plans;
        //Explorer * myexplorer;
        for (int i = 0; i < entityCount; i++) {
            string entityType;
            int id;
            int x;
            int y;
            int param0;
            int param1;
            int param2;
            cin >> entityType >> id >> x >> y >> param0 >> param1 >> param2; cin.ignore();
            if (entityType == "EXPLORER") {
                Explorer foo (x, y, id, param0, param1, param2);
                explorers.push_back(foo);
                //if (i == 0) {myexplorer = &explorers.front();}
            } else if (entityType == "WANDERER") {
                Wanderer foo (x, y, id, param0, param1, param2);
                wanderers.push_back(foo);
            } else if (entityType == "SLASHER") {
                Slasher foo (x, y, id, param0, param1, param2);
                slashers.push_back(foo);
            } else if (entityType == "EFFECT_SHELTER") {
                Shelter foo(x, y, id, param0);
                shelters.push_back(foo);
            } else if (entityType == "EFFECT_PLAN") {
                Plan foo(x, y, id, param0);
                plans.push_back(foo);
            }
            
        }
        board.set_turnDanger(explorers, wanderers, slashers, shelters, plans);
        //board.print_Board();
        //logic
        
        string str = "";
        if (explorers.front().yell_check(explorers, wanderers)) {
            str = "YELL BEHIND YOU!";
            for (Explorer e : explorers) {
                if (explorers.front().get_distance(e) < 2) {
                    yelledat.push_back(e.get_id());
                }
            }
        } else if (explorers.front().plan_check(explorers, wanderers)) {
            str = "PLAN Lets work together.";
        } else if (explorers.front().light_check(explorers, wanderers)) {
            str = "LIGHT A light in the darkness";
        } else {
            str = board.get_movement(explorers.front());
        }
        cout << str << endl;
        
    }
}
