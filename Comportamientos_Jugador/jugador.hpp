#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"
#include <list>
#include <queue>

#define BATTERY_THRESH 2500

using namespace std;

enum Vision {leftdiagonal, straight, rightdiagonal };

struct Square
{
    int fil;
    int col;
    Square() : fil(0), col(0){}
    Square(int a, int b) : fil(a), col(b){}
    Square operator=(const Square & sq) {
        this->fil = sq.fil;
        this->col = sq.col;
        return *this;
    }
    bool operator==(const Square & other)
    {
        return fil == other.fil && col == other.col;
    }
};

struct State : public Square
{
    Orientacion brujula;
};

struct Movement : public Square
{
    
};

class ComportamientoJugador : public Comportamiento
{

  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size){
        printCliffs();
        current_state.fil = 99;
        current_state.col = 99;
        current_state.brujula = norte;
        last_action = actIDLE;
        bien_situado = false;
        bikini = false;
        zapatillas = false;
        need_reload = false;
        wall_protocol = false;
        cont_actWALK = 0;
        aux_map.resize(200, vector<unsigned char>(200, '?'));
        aux_prio.resize(200, vector<unsigned int>(200, 0));
        priority.resize(mapaResultado.size(), vector<unsigned int>(mapaResultado.size(), 0));
    }

    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
    void printSensors(const Sensores & sensores);
    void printMap(const vector<unsigned char> & terreno, const State & st, vector<vector<unsigned char>> & map, vector<vector<unsigned int>> & prio, bool faulty);
    void printPriorities(const vector<unsigned char> & terreno, const State & st, vector<vector<unsigned int>> & prio);
    bool detectBikini(const vector<unsigned char> & terreno);
    bool detectZapatillas(const vector<unsigned char> & terreno);
    bool detectReload(const vector<unsigned char> & terreno);
    bool detectPositioning(const vector<unsigned char> & terreno);
    bool detectWalls(const vector<unsigned char> & terreno);
    bool alreadyExplored(const vector<vector<unsigned char>> & map, const State & st, const Action & a);
    Movement setMovement(const Orientacion & brujula, const Vision & view);
    Action selectMovement(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, vector<vector<unsigned int>> & prio);
    bool canMoveDiagonally(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, vector<vector<unsigned int>> & prio, Vision vision, unsigned int & min);
    bool accesibleSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, int index);
    void translateMap(const Sensores & sensores, const State & st, const vector<vector<unsigned char>> & aux, vector<vector<unsigned char>> & map);
    void rotateMap(const Sensores & sensores, const State & st, vector<vector<unsigned char>> & map);
    Square searchUnexplored(const vector<vector<unsigned char>> & map);
    int measureDistance(const Square & sq1, const Square & sq2);
    int relativePosition(const Square sq1, const Square & sq2);
    int setPriority(const unsigned char & sq);
    void modifyPriority(const vector<vector<unsigned char>> & map, bool aux);
    Action wallProtocol(const vector<unsigned char> & terreno);
    Action goToLocation(const Square & location, const vector<unsigned char> & terreno, const vector<unsigned char> & agentes);
    Action searchSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, unsigned char square);
    Action rotate();

  private:
    State current_state;
    Action last_action;
    Square objective;
    bool bien_situado;
    bool bikini;
    bool zapatillas;
    bool need_reload;
    bool goto_objective;
    bool wall_protocol;
    Vision had_walls;
    int discovered;
    int cont_actWALK;
    vector<vector<unsigned char>> aux_map;
    vector<vector<unsigned int>> aux_prio;
    vector<vector<unsigned int>> priority;
    list<Action> protocol;
    void resetState();
    void printCliffs();
};
#endif
