#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>
using namespace std;

Action ComportamientoJugador::think(Sensores sensores)
{

	Action accion = actIDLE;

	// Mostrar el valor de los sensores
	cout << "Posicion: fila " << sensores.posF << " columna " << sensores.posC;
	switch (sensores.sentido)
	{
		case norte:	  cout << " Norte\n";	break;
		case noreste: cout << " Noreste\n";	break;
		case este:    cout << " Este\n";	break;
		case sureste: cout << " Sureste\n";	break;
		case sur:     cout << " Sur\n";	break;
		case suroeste:cout << " Suroeste\n";	break;
		case oeste:   cout << " Oeste\n";	break;
		case noroeste:cout << " Noroeste\n";	break;
	}
	cout << "Terreno: ";
	for (int i=0; i<sensores.terreno.size(); i++)
		cout << sensores.terreno[i];

	cout << "  Agentes: ";
	for (int i=0; i<sensores.agentes.size(); i++)
		cout << sensores.agentes[i];

	cout << "\nColision: " << sensores.colision;
	cout << "  Reset: " << sensores.reset;
	cout << "  Vida: " << sensores.vida << endl<< endl;

    // Fase de observación
    switch (last_action)
    {
        case actWALK:
            switch (current_state.brujula)
            {
                case norte: 
                    current_state.fil--;
                break;
                case noreste:
                    current_state.fil--;
                    current_state.col++;
                break;
                case este:
                    current_state.col++;
                break;
                case sureste:
                    current_state.fil++;
                    current_state.col++;
                break;
                case sur:
                    current_state.fil++;
                break;
                case suroeste:
                    current_state.fil++;
                    current_state.col--;
                break;
                case oeste:
                    current_state.col--;
                break;
                case noroeste:
                    current_state.fil--;
                    current_state.col--;
                break;
            }
        break;
        case actRUN:
            // Implementar
        break;
        case actTURN_SR:
            current_state.brujula = static_cast<Orientacion> ((current_state.brujula+1)%8);
            // girar_derecha = (rand() % 2 == 0);
        break;
        case actTURN_L:
            current_state.brujula = static_cast<Orientacion> ((current_state.brujula+6)%8);
            // girar_derecha = (rand() % 2 == 0);
        break;
    }

    if (sensores.terreno[0] == 'G' && !bien_situado)
    {
        current_state.fil = sensores.posF;
        current_state.col = sensores.posC;
        current_state.brujula = sensores.sentido;
        bien_situado = true;
    }

    if (bien_situado)
    {
        PonerTerrenoEnMatriz(sensores.terreno, current_state, mapaResultado);
    }

    // Fase de decisión de la nueva acción
    if ((sensores.terreno[2] == 'T' || sensores.terreno[2] == 'S' || sensores.terreno[2] == 'G') && sensores.agentes[2] == '_')
    {
        accion = actWALK;
    }
    else if (!girar_derecha)
    {
        accion = actTURN_L;
        girar_derecha = (rand() % 2 == 0);
    }
    else
    {
        accion = actTURN_SR;
        girar_derecha = (rand() % 2 == 0);
    }
    // Devuelve el valor de la acción
    last_action = accion;
	return accion;
}

void ComportamientoJugador::PonerTerrenoEnMatriz(const vector<unsigned char> & terreno, const state & st, vector<vector<unsigned char>> & m)
{
    m[st.fil][st.col] = terreno[0];
}

int ComportamientoJugador::interact(Action accion, int valor)
{
	return false;
}