#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"
#include <set>
#include <queue>

using namespace std;

struct state{
    int fil;
    int col;
    Orientacion brujula;
};

struct Movement{
    int fil;
    int col;
    Movement(int a = 0, int b = 0) : fil(a), col(b){}
};

class ComportamientoJugador : public Comportamiento{

  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size){
      current_state.fil = 99;
      current_state.col = 99;
      current_state.brujula = norte;
      last_action = actIDLE;
      bien_situado = false;
      cont_actWALK = 0;
      aux_map.resize(200, vector<unsigned char>(200, '?'));
    }

    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
    void printSensors(const Sensores & sensores);
    void PonerTerrenoEnMatriz(const vector<unsigned char> & terreno, const state & st, vector<vector<unsigned char>> & map);
    bool detectBikini(const vector<unsigned char> & terreno);
    bool detectZapatillas(const vector<unsigned char> & terreno);
    bool detectReload(const vector<unsigned char> & terreno);
    bool detectPositioning(const vector<unsigned char> & terreno);
    bool alreadyExplored(const vector<vector<unsigned char>> & map, const state & st, const Action & a);
    Movement moveForward(const Orientacion & brujula);
    bool canMoveForward(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes);
    bool canMoveDiagonal(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes);
    void translateMap(const Sensores & sensores, const state & st, const vector<vector<unsigned char>> & aux, vector<vector<unsigned char>> & map);
    Movement searchUnexplored();
    float measure(const state & st1, const state & st2);
    queue<Action> goToLocation(Movement location);
    bool withinLimits(int i, int j);
    Action rotate();

  private:
    state current_state;
    Action last_action;
    bool bien_situado;
    bool bikini;
    bool zapatillas;
    vector<vector<unsigned char>> aux_map;
    set<state> reloadsLocation;
    set<state> bikinisLocation;
    set<state> zapatillasLocation;
    queue<Action> protocol;
    int cont_actWALK;
};
#endif
