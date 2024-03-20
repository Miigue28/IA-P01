#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"

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
    }

    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
    void printSensors(const Sensores & sensores);
    void PonerTerrenoEnMatriz(const vector<unsigned char> & terreno, const state & st, vector<vector<unsigned char>> & map);
    bool specialItems(const vector<unsigned char> & terreno);
    bool alreadyExplored(const vector<vector<unsigned char>> & map, const state & st, const Action & a);
    Movement moveForward(const Orientacion & brujula);
    bool canMoveForward(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes);
    bool canMoveDiagonal(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes);
    Action rotate();

  private:
    state current_state;
    Action last_action;
    bool bien_situado;
    int cont_actWALK;
};
#endif
