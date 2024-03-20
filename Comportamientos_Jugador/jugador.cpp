#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>

using namespace std;

Action ComportamientoJugador::think(Sensores sensores)
{
	Action accion = actIDLE;

	printSensors(sensores);

    Movement move;

    // Fase de observación
    switch (last_action)
    {
        case actWALK:
            move = moveForward(current_state.brujula);
            current_state.fil += move.fil;
            current_state.col += move.col;
        break;
        case actRUN:
            move = moveForward(current_state.brujula);
            current_state.fil += 2*move.fil;
            current_state.col += 2*move.col;
        break;
        case actTURN_SR:
            current_state.brujula = static_cast<Orientacion> ((current_state.brujula+1)%8);
        break;
        case actTURN_L:
            current_state.brujula = static_cast<Orientacion> ((current_state.brujula+6)%8);
        break;
    }

    if ((sensores.terreno[0] == 'G' || sensores.nivel == 0) && !bien_situado)
    {
        translateMap(sensores, current_state, aux_map, mapaResultado);
        current_state.fil = sensores.posF;
        current_state.col = sensores.posC;
        current_state.brujula = sensores.sentido;
        bien_situado = true;
    }

    if (sensores.reset)
    {
        current_state.fil = 99;
        current_state.col = 99;
        current_state.brujula = norte;
        bien_situado = false;
        aux_map.clear();
    }

    if (bien_situado)
        PonerTerrenoEnMatriz(sensores.terreno, current_state, mapaResultado);
    else
        PonerTerrenoEnMatriz(sensores.terreno, current_state, aux_map);

    // Fase de decisión de la nueva acción
    /*
    if (!bien_situado && detectPositioning(sensores.terreno))
    {

    }
    else
    */
    if (canMoveForward(sensores.terreno, sensores.agentes) && cont_actWALK < 15)
    {
        accion = actWALK;
        cont_actWALK++;
    }
    else
    {
        accion = rotate();
        cont_actWALK = 0;
    }
    // Devuelve el valor de la acción
    last_action = accion;
	return accion;
}

void ComportamientoJugador::PonerTerrenoEnMatriz(const vector<unsigned char> & terreno, const state & st, vector<vector<unsigned char>> & map)
{
    int k = 0;
    map[st.fil][st.col] = terreno[k++];
    switch (st.brujula)
    {
        case norte:
            for (int i = 1; i < 4; i++)
                for (int j = 1; j < 2*i+2; j++)
                    map[st.fil - i][st.col - i - 1 + j] = terreno[k++];
        break;
        case noreste:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i + 2; j++)
                {
                    if (j < i + 1)
                        map[st.fil - i][st.col + j - 1] = terreno[k++];
                    else   
                        map[st.fil - i - i - 1 + j][st.col + i] = terreno[k++];
                }
            }     
        break;
        case este:
            for (int i = 1; i < 4; i++)
                for (int j = 1; j < 2*i+2; j++)
                    map[st.fil - i - 1 + j][st.col + i] = terreno[k++];
        break;
        case sureste:
        for (int i = 1; i < 4; i++)
        {
          for (int j = 1; j < 2*i+2; j++)
          {
            if (j < i + 1)
                map[st.fil + j - 1][st.col + i] = terreno[k++];
            else   
                map[st.fil + i][st.col + i + i + 1 - j] = terreno[k++];
          }
        }    
        break;
        case sur:
            for (int i = 1; i < 4; i++)
                for (int j = 1; j < 2*i+2; j++)
                    map[st.fil + i][st.col + i + 1 - j] = terreno[k++];
        break;
        case suroeste:
        for (int i = 1; i < 4; i++)
        {
          for (int j = 1; j < 2*i+2; j++)
          {
            if (j < i + 1)
                map[st.fil + i][st.col - j + 1] = terreno[k++];
            else   
                map[st.fil + i + i + 1 - j][st.col - i] = terreno[k++];
          }
        }    
        break;
        case oeste:
            for (int i = 1; i < 4; i++)
                for (int j = 1; j < 2*i+2; j++)
                    map[st.fil + i + 1 - j][st.col - i] = terreno[k++];
        break;
        case noroeste:
        for (int i = 1; i < 4; i++)
        {
          for (int j = 1; j < 2*i+2; j++)
          {
            if (j < i + 1)
                map[st.fil - j + 1][st.col - i] = terreno[k++];
            else   
                map[st.fil - i][st.col - i - i - 1 + j] = terreno[k++];
          }
        }  
        break;
    }
}

void ComportamientoJugador::translateMap(const Sensores & sensores, const state & st, const vector<vector<unsigned char>> & aux, vector<vector<unsigned char>> & map)
{
    int row = abs(st.fil - sensores.posF);
    int column = abs(st.col - sensores.posC);
    
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size(); j++)
        {
            if (map[i][j] == '?')
                map[i][j] = aux[row + i][column + j];
        }
    }        
}


bool ComportamientoJugador::detectBikini(const vector<unsigned char> & terreno)
{
    for (auto t : terreno)
    {
        if (t == 'B')
            return true;
    }
    return false;
}

bool ComportamientoJugador::detectPositioning(const vector<unsigned char> & terreno)
{
    for (auto t : terreno)
    {
        if (t == 'G')
            return true;
    }
    return false;
}

queue<Action> ComportamientoJugador::goToLocation(Movement location)
{
    return {};
}

Movement ComportamientoJugador::searchUnexplored()
{
    int r = current_state.fil;
    int c = current_state.col;
    int rk = 0, ck = 0;
    for (int k = 0; ; k++)
    {
        // CONSIDERAR LOS CASOS CERCANOS A LOS BORDES
        for (int i = r - rk; i < (r + k) % mapaResultado.size(); i++)
        {
            for (int j = c - ck; j < (c + k) % mapaResultado.size(); j++)
            {
                if (mapaResultado[i][j] == '?')
                    return {i, j};
            }
        }
    }
    return {};
}

bool ComportamientoJugador::withinLimits(int i, int j)
{
    bool row = (0 <= i && i <= mapaResultado.size());
    bool column = (0 <= j && j <= mapaResultado.size());
    return (row && column);
}

Action ComportamientoJugador::rotate()
{
    return (rand() % 2 == 0) ? actTURN_SR : actTURN_L;
}

Movement ComportamientoJugador::moveForward(const Orientacion & brujula)
{
    Movement move;
    switch (brujula)
    {
        case norte: 
            move.fil--;
        break;
        case noreste:
            move.fil--;
            move.col++;
        break;
        case este:
            move.col++;
        break;
        case sureste:
            move.fil++;
            move.col++;
        break;
        case sur:
            move.fil++;
        break;
        case suroeste:
            move.fil++;
            move.col--;
        break;
        case oeste:
            move.col--;
        break;
        case noroeste:
            move.fil--;
            move.col--;
        break;
    }
    return move;
}

bool ComportamientoJugador::canMoveForward(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    return (terreno[2] != 'P' && terreno[2] != 'M' && agentes[2] == '_');
}

bool ComportamientoJugador::canMoveDiagonal(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    return (terreno[3] != 'P' && terreno[3] != 'M' && agentes[3] == '_');
}

void ComportamientoJugador::printSensors(const Sensores & sensores)
{
    // Muestra el valor de los sensores
	std::cout << "Posicion: fila " << sensores.posF << " columna " << sensores.posC;
	switch (sensores.sentido)
	{
		case norte:	  std::cout << " Norte\n";	break;
		case noreste: std::cout << " Noreste\n";	break;
		case este:    std::cout << " Este\n";	break;
		case sureste: std::cout << " Sureste\n";	break;
		case sur:     std::cout << " Sur\n";	break;
		case suroeste:std::cout << " Suroeste\n";	break;
		case oeste:   std::cout << " Oeste\n";	break;
		case noroeste:std::cout << " Noroeste\n";	break;
	}
	std::cout << "Terreno: ";
	for (int i = 0; i < sensores.terreno.size(); i++)
		std::cout << sensores.terreno[i];

	std::cout << "  Agentes: ";
	for (int i = 0; i < sensores.agentes.size(); i++)
		std::cout << sensores.agentes[i];

	std::cout << "\nColision: " << sensores.colision;
	std::cout << "  Reset: " << sensores.reset;
	std::cout << "  Vida: " << sensores.vida << endl << endl;
}

int ComportamientoJugador::interact(Action accion, int valor)
{
	return false;
}
