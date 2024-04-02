#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>

using namespace std;

Action ComportamientoJugador::think(Sensores sensores)
{
	Action action = actIDLE;

	printSensors(sensores);

    Movement move;
    const int DISCOVERED_THRESH = int(mapaResultado.size()*mapaResultado.size()*0.75);
    bool faulty = (sensores.nivel == 3);

    // Fase de observación
    switch (last_action)
    {
        case actWALK:
            move = setMovement(current_state.brujula, straight);
            current_state.fil += move.fil;
            current_state.col += move.col;
        break;
        case actRUN:
            move = setMovement(current_state.brujula, straight);
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

    if (sensores.nivel == 0)
    {
        current_state.fil = sensores.posF;
        current_state.col = sensores.posC;
        current_state.brujula = sensores.sentido;
        bien_situado = true;
    }

    if (sensores.reset)
    {
        resetState();
    }

    if (bien_situado)
        printMap(sensores.terreno, current_state, mapaResultado, priority, faulty);
    else
        printMap(sensores.terreno, current_state, aux_map, aux_prio, faulty);


    switch (sensores.terreno[0])
    {
        case 'G':
            if (!bien_situado)
            {
                translateMap(sensores, current_state, aux_map, mapaResultado);
                // TODO: Verificar si el número de giros es el correcto
                rotateMap(sensores, current_state, mapaResultado);
                current_state.fil = sensores.posF;
                current_state.col = sensores.posC;
                current_state.brujula = sensores.sentido;
                bien_situado = true;
            }
        break;
        case 'X':
            need_reload = false;
            // Nos esperamos hasta que recarguemos suficientemente la batería
            if (sensores.bateria < 4500)
            {
                last_action = action;
                return action;
            }
        break;
        case 'K':
            if (bien_situado && !bikini) modifyPriority(mapaResultado, false); else modifyPriority(aux_map, true);
            bikini = true;
        break;
        case 'D':
            if (bien_situado && !zapatillas) modifyPriority(mapaResultado, false); else modifyPriority(aux_map, true);
            zapatillas = true;
        break;
        default:
            if (bien_situado)
                priority[current_state.fil][current_state.col]++;
            else
                aux_prio[current_state.fil][current_state.col]++;
        break;
    }

    // Si estamos escasos de batería vamos a la casilla de recarga más cercana
    if (sensores.bateria < BATTERY_THRESH)
    {
        need_reload = true;
    }

    if (sensores.terreno[2] == 'M')
        wall_protocol = true;

    // Fase de decisión de la nueva acción
    if (!protocol.empty())
    {
        action = protocol.front();
        protocol.pop_front();
    }
    else if (wall_protocol)
    {
        action = wallProtocol(sensores.terreno);
    }
    else if (!bien_situado && detectPositioning(sensores.terreno))
    {
        action = searchSquare(sensores.terreno, sensores.agentes, 'G');
    }
    else if (sensores.bateria < 4000 && detectReload(sensores.terreno))
    {
        action = searchSquare(sensores.terreno, sensores.agentes, 'X');
    }
    else if (!bikini && detectBikini(sensores.terreno))
    {
        action = searchSquare(sensores.terreno, sensores.agentes, 'K');
    }
    else if (!zapatillas && detectZapatillas(sensores.terreno))
    {
        action = searchSquare(sensores.terreno, sensores.agentes, 'D'); 
    }
    else if (!bikini || !zapatillas)
    {
        action = selectMovement(sensores.terreno, sensores.agentes, bien_situado ? priority : aux_prio);
    }
    //else if (need_reload && !reloads.empty())
    //{
    //    // Calculamos la recarga más cercana
    //    int distance = -1, min = 500;
    //    for (auto r : reloads)
    //    {
    //        distance = measureDistance((Square)current_state, r);
    //        if (distance < min)
    //        {
    //            objective = r;
    //            min = distance; 
    //        }
    //    }
    //    action = goToLocation(objective, sensores.terreno, sensores.agentes);
    //}
    //else if (discovered > DISCOVERED_THRESH)
    //{
    //    objective = searchUnexplored(mapaResultado);
    //    action = goToLocation(objective, sensores.terreno, sensores.agentes);
    //}
    else if (accesibleSquare(sensores.terreno, sensores.agentes, 2) && cont_actWALK < 15)
    {
        action = actWALK;
        cont_actWALK++;
    }
    else
    {
        action = rotate();
        cont_actWALK = 0;
    }
    
    // Devuelve el valor de la acción
    last_action = action;
	return action;
}

bool ComportamientoJugador::detectBikini(const vector<unsigned char> & terreno)
{
    for (auto t : terreno)
    {
        if (t == 'K')
            return true;
    }
    return false;
}

bool ComportamientoJugador::detectZapatillas(const vector<unsigned char> & terreno)
{
    for (auto t : terreno)
    {
        if (t == 'D')
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

bool ComportamientoJugador::detectReload(const vector<unsigned char> & terreno)
{
    for (auto t : terreno)
    {
        if (t == 'X')
            return true;
    }
    return false;
}

bool ComportamientoJugador::detectWalls(const vector<unsigned char> & terreno)
{
    for (int i = 0; i < 3; i++)
    {
        if (terreno[i+1] == 'M')
            return true;
    }
    return false;
}

Action ComportamientoJugador::searchSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, unsigned char square)
{
    // HACER MÁS EFICIENTE ESTÁ BUSQUEDA
    int index = -1;
    for (int i = 0; i < terreno.size(); i++)
    {
        if (terreno[i] == square)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        return actIDLE;
    }

    if (1 <= index && index <= 3)
    {
        if (index == 2)
            return actWALK;
        else if (index == 3)
            return actTURN_SR;
        else
            return actTURN_L;
    }
    else if (4 <= index && index <= 8)
    {
        if (index == 6)
            return actRUN;
        else if (index < 6)
            return actWALK;
        else
            return actTURN_SR;
    }
    else
    {
        if (index <= 12)
            return (accesibleSquare(terreno, agentes, 4) ? actRUN : actWALK);
        else
            return actTURN_SR;
    }
}

/*
Action ComportamientoJugador::goToLocation(const Square & location, const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    int row = current_state.fil;
    int column = current_state.col;
    int relative_position = -1;
    if (row < location.fil)
    {
        if (column < location.col)
        {
            relative_position = 0;
        }
        else
        {
            relative_position = 1;
        }
    }
    else 
    {
        if (column < location.col)
        {
            relative_position = 2;
        }
        else
        {
            relative_position = 3;
        }
    }

    switch (current_state.brujula)
    {
        case norte:
            switch (relative_position)
            {
                case 0:
                    return actTURN_SR;
                break;
                case 1:
                    return actTURN_L;
                break;
                case 2:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 3:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
            }
        break;
        case noreste:
            switch (relative_position)
            {
                case 0:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 1:
                    return actTURN_L;
                break;
                case 2:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 3:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
            }
        break;
        case este:
            switch (relative_position)
            {
                case 0:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 1:
                    return actTURN_SR;
                break;
                case 2:
                    return actTURN_L;
                break;
                case 3:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
            }
        break;
        case sureste:
            switch (relative_position)
            {
                case 0:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 1:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 2:
                    return actTURN_L;
                break;
                case 3:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
            }
        break;
        case sur:
            switch (relative_position)
            {
                case 0:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 1:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 2:
                    return actTURN_L;
                break;
                case 3:
                    return actTURN_SR;
                break;
            }
        break;
        case suroeste:
            switch (relative_position)
            {
                case 0:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 1:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 2:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 3:
                    return actTURN_L;
                break;
            }
        break;
        case oeste:
            switch (relative_position)
            {
                case 0:
                    return actTURN_L;
                break;
                case 1:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 2:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 3:
                    return actTURN_SR;
                break;
            }
        break;
        case noroeste:
            switch (relative_position)
            {
                case 0:
                    return actTURN_L;
                break;
                case 1:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_L;
                break;
                case 2:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
                case 3:
                    if (canMoveForward(terreno, agentes))
                        return actWALK;
                    else
                        return actTURN_SR;
                break;
            }
        break;
    }
    return {};
}
*/

Action ComportamientoJugador::wallProtocol(const vector<unsigned char> & terreno)
{
    if (terreno[1] == 'M' && terreno[2] == 'M' && terreno[3] == 'M')
        return actTURN_L;
    else if (terreno[1] == 'M' && terreno[2] == 'M')
        return actTURN_SR;
    else if (terreno[2] == 'M' && terreno[3] == 'M')
        return actTURN_L;
    else if (terreno[1] == 'M' && terreno[3] == 'M')
        return actWALK;
    else if (terreno[1] == 'M' || terreno[3] == 'M')
    {
        had_walls = (terreno[1] == 'M' ? leftdiagonal : rightdiagonal);
        return actWALK;
    }
    else
    {
        wall_protocol = false;
        switch (had_walls)
        {
            case rightdiagonal:
                protocol.emplace_back(actWALK);
                return actTURN_SR;
            case leftdiagonal:
                protocol.emplace_back(actTURN_L);
                protocol.emplace_back(actWALK);
                return actWALK;
        }
    }
}

Square ComportamientoJugador::searchUnexplored(const vector<vector<unsigned char>> & map)
{
    int row = current_state.fil;
    int column = current_state.col;
    int row_lowerbound = current_state.fil;
    int column_lowerbound = current_state.col;
    int row_upperbound = current_state.fil;
    int column_upperbound = current_state.col;
    int min = 500;
    int n = mapaResultado.size();
    int distance;
    bool found = false;

    Square out, tmp;
    Square base(row, column);
    
    for (int k = 1; !found && (row_lowerbound != 0 || column_lowerbound != 0 || row_upperbound != n-1 || column_upperbound != n-1); k++)
    {
        row_lowerbound = ((0 <= row - k) ? (row - k) : row_lowerbound);
        column_lowerbound = ((0 <= column - k) ? (column - k) : column_lowerbound);
        row_upperbound = ((row + k < n) ? (row + k) : row_upperbound);
        column_upperbound = ((column + k < n) ? (column + k) : column_upperbound);

        for (int i = row_lowerbound; i <= row_upperbound; i++)
        {
            if (map[i][column_lowerbound] == '?')
            {
                tmp = {i, column_lowerbound};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
            if (map[i][column_upperbound] == '?')
            {
                tmp = {i, column_upperbound};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
        }
        for (int i = column_lowerbound; i <= column_upperbound; i++)
        {
            if (map[row_lowerbound][i] == '?')
            {
                tmp = {row_lowerbound, i};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
            if (map[row_upperbound][i] == '?')
            {
                tmp = {row_upperbound, i};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
        }
        if (min != 500)
        {
            found = true;
        }
    }

    return out;
}

int ComportamientoJugador::measureDistance(const Square & st1, const Square & st2)
{
    return abs(st1.fil - st2.fil) + abs(st1.col - st2.col);
}

Action ComportamientoJugador::rotate()
{
    return (rand() % 2 == 0) ? actTURN_SR : actTURN_L;
}

int ComportamientoJugador::setPriority(const unsigned char & sq)
{
    switch (sq)
    {
        case 'A': return 20; break;
        case 'B': return 30; break;
        case 'P': return 100; break;
        case 'M': return 100; break;
        case 'T': return 10; break;
        case 'S': return 5; break;
        case 'G': return 5; break;
        case 'K': return 5; break;
        case 'D': return 5; break;
        case 'X': return 5; break;
    }
}

void ComportamientoJugador::modifyPriority(const vector<vector<unsigned char>> & map, bool aux)
{
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size(); j++)
        {
            if (bikini && map[i][j] == 'A')
            {
                if (!aux)
                    priority[i][j] -= 10;
                else
                    aux_prio[i][j] -= 10;
            }
            if (zapatillas && map[i][j] == 'B')
            {
                if (!aux)
                    priority[i][j] -= 15;
                else
                    aux_prio[i][j] -= 15;
            }
        }
    }
}

Action ComportamientoJugador::selectMovement(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, vector<vector<unsigned int>> & prio)
{
    int walk = 2, run = 6;
    unsigned int min = 500, min_leftdiag = 500, min_rightdiag = 500;
    Movement move = setMovement(current_state.brujula, straight);
    Action action = actTURN_L;
    if (accesibleSquare(terreno, agentes, walk))
    {
        min = prio[current_state.fil + move.fil][current_state.col + move.col];
        action = actWALK;
        // Si la siguiente también es accesible
        if (accesibleSquare(terreno, agentes, run))
        {
            unsigned int run_prio = prio[current_state.fil + 2*move.fil][current_state.col + 2*move.col];
            if (run_prio <= min)
            {
                min = run_prio;
                action = actRUN;
            }
        }
    }
    if (canMoveDiagonally(terreno, agentes, bien_situado ? priority : aux_prio, rightdiagonal, min_rightdiag) && min_rightdiag < min)
    {
        min = min_rightdiag;
        action = actTURN_SR;
    }
    if (canMoveDiagonally(terreno, agentes, bien_situado ? priority : aux_prio, leftdiagonal, min_leftdiag) && min_leftdiag < min)
    {
        min = min_leftdiag;
        action = actTURN_L;
    }
    return action;
}

bool ComportamientoJugador::canMoveDiagonally(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, vector<vector<unsigned int>> & prio, Vision vision, unsigned int & min)
{
    int walk, run;
    switch (vision)
    {
        case leftdiagonal: walk = 1; run = 4; break;
        case rightdiagonal: walk = 3; run = 8; break;
    }
    Movement move = setMovement(current_state.brujula, vision);
    // Si la proxima casilla es accesible
    if (accesibleSquare(terreno, agentes, walk))
    {
        min = prio[current_state.fil + move.fil][current_state.col + move.col];
        // Si la siguiente también es accesible
        if (accesibleSquare(terreno, agentes, run))
        {
            unsigned int run_prio = prio[current_state.fil + 2*move.fil][current_state.col + 2*move.col];
            min = run_prio < min ? run_prio : min;
        }
        return true;
    }

    return false;
}

bool ComportamientoJugador::accesibleSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, int index)
{
    return (terreno[index] != 'P' && terreno[index] != 'M' && agentes[index] == '_');
}

Movement ComportamientoJugador::setMovement(const Orientacion & brujula, const Vision & view)
{
    Movement move;
    switch(view)
    {
        case straight:
            switch (brujula)
            {
                case norte: move.fil--; break;
                case noreste: move.fil--; move.col++; break;
                case este: move.col++; break;
                case sureste: move.fil++; move.col++; break;
                case sur: move.fil++; break;
                case suroeste: move.fil++; move.col--; break;
                case oeste: move.col--; break;
                case noroeste: move.fil--; move.col--; break;
            }
        break;
        case leftdiagonal:
            switch (brujula)
            {
                case norte: move.fil--; move.col--; break;
                case noreste: move.fil--; break;
                case este: move.fil--; move.col++; break;
                case sureste: move.col++; break;
                case sur: move.fil++; move.col++; break;
                case suroeste: move.fil++; break;
                case oeste: move.fil++; move.col--; break;
                case noroeste: move.col--; break;
            }
        break;
        case rightdiagonal:
            switch (brujula)
            {
                case norte: move.fil--; move.col++; break;
                case noreste: move.col++; break;
                case este: move.fil++; move.col++; break;
                case sureste: move.fil++; break;
                case sur: move.fil++; move.col--; break;
                case suroeste: move.col--; break;
                case oeste: move.fil--; move.col--; break;
                case noroeste: move.fil--; break;
            }
        break;
    }
    return move;
}

void ComportamientoJugador::printMap(const vector<unsigned char> & terreno, const State & st, vector<vector<unsigned char>> & map, vector<vector<unsigned int>> & prio, bool faulty)
{
    int k = 0;
    map[st.fil][st.col] = terreno[k];
    prio[st.fil][st.col] = setPriority(terreno[k]);
    k++;
    switch (st.brujula)
    {
        case norte:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                    {
                        k++;
                        continue;
                    }

                    if (map[st.fil - i][st.col - i - 1 + j] == '?')
                    {
                        map[st.fil - i][st.col - i - 1 + j] = terreno[k];
                        prio[st.fil - i][st.col - i - 1 + j] = setPriority(terreno[k]);
                    }
                    k++;
                }
            }  
        break;
        case noreste:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i + 2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                    {
                        k++;
                        continue;
                    }

                    if (j < i + 1)
                    {
                        if (map[st.fil - i][st.col + j - 1] == '?')
                        {
                            map[st.fil - i][st.col + j - 1] = terreno[k];
                            prio[st.fil - i][st.col + j - 1] = setPriority(terreno[k]);
                        }
                        k++;
                    }
                    else
                    {
                        if (map[st.fil - i - i - 1 + j][st.col + i] == '?')
                        {
                            map[st.fil - i - i - 1 + j][st.col + i] = terreno[k];
                            prio[st.fil - i - i - 1 + j][st.col + i] = setPriority(terreno[k]);
                        }
                        k++;
                    }  
                }
            }     
        break;
        case este:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                    {
                        k++;
                        continue;
                    }

                    if (map[st.fil - i - 1 + j][st.col + i] == '?')
                    {
                        map[st.fil - i - 1 + j][st.col + i] = terreno[k];
                        prio[st.fil - i - 1 + j][st.col + i] = setPriority(terreno[k]);
                    }
                    k++;
                }
            }      
        break;
        case sureste:
        for (int i = 1; i < 4; i++)
        {
          for (int j = 1; j < 2*i+2; j++)
          {
            if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
            {
                k++;
                continue;
            }

            if (j < i + 1)
            {
                if (map[st.fil + j - 1][st.col + i] == '?')
                {
                    map[st.fil + j - 1][st.col + i] = terreno[k];
                    prio[st.fil + j - 1][st.col + i] = setPriority(terreno[k]);
                }
                k++;
            }
            else
            {
                if (map[st.fil + i][st.col + i + i + 1 - j] == '?')
                {
                    map[st.fil + i][st.col + i + i + 1 - j] = terreno[k];
                    prio[st.fil + i][st.col + i + i + 1 - j] = setPriority(terreno[k]);
                }
                k++;
            }
          }
        }    
        break;
        case sur:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                    {
                        k++;
                        continue;
                    }
                    
                    if (map[st.fil + i][st.col + i + 1 - j] == '?')
                    {
                        map[st.fil + i][st.col + i + 1 - j] = terreno[k];
                        prio[st.fil + i][st.col + i + 1 - j] = setPriority(terreno[k]);
                    }
                    k++;
                }
            }
        break;
        case suroeste:
            for (int i = 1; i < 4; i++)
            {
              for (int j = 1; j < 2*i+2; j++)
              {
                if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                {
                    k++;
                    continue;
                }

                if (j < i + 1)
                {
                    if (map[st.fil + i][st.col - j + 1] == '?')
                    {
                        map[st.fil + i][st.col - j + 1] = terreno[k];
                        prio[st.fil + i][st.col - j + 1] = setPriority(terreno[k]);
                    }
                    k++;
                }
                else
                {
                    if (map[st.fil + i + i + 1 - j][st.col - i] == '?')
                    {
                        map[st.fil + i + i + 1 - j][st.col - i] = terreno[k];
                        prio[st.fil + i + i + 1 - j][st.col - i] = setPriority(terreno[k]);
                    }
                    k++;
                }
              }
            }    
        break;
        case oeste:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                    {
                        k++;
                        continue;
                    }

                    if (map[st.fil + i + 1 - j][st.col - i] == '?')
                    {
                        map[st.fil + i + 1 - j][st.col - i] = terreno[k];
                        prio[st.fil + i + 1 - j][st.col - i] = setPriority(terreno[k]);
                    }
                    k++;
                }
            }
        break;
        case noroeste:
        for (int i = 1; i < 4; i++)
        {
          for (int j = 1; j < 2*i+2; j++)
          {
            if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
            {
                k++;
                continue;
            }

            if (j < i + 1)
            {
                if (map[st.fil - j + 1][st.col - i] == '?')
                {
                    map[st.fil - j + 1][st.col - i] = terreno[k];
                    prio[st.fil - j + 1][st.col - i] = setPriority(terreno[k]);
                }
                k++;
            }
            else
            {
                if (map[st.fil - i][st.col - i - i - 1 + j] == '?')
                {
                    map[st.fil - i][st.col - i - i - 1 + j] = terreno[k];
                    prio[st.fil - i][st.col - i - i - 1 + j] = setPriority(terreno[k]);
                }
                k++;
            }
          }
        }  
        break;
    }
}

void ComportamientoJugador::translateMap(const Sensores & sensores, const State & st, const vector<vector<unsigned char>> & aux, vector<vector<unsigned char>> & map)
{
    int row = abs(st.fil - sensores.posF);
    int column = abs(st.col - sensores.posC);
    
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size(); j++)
        {
            if (map[i][j] == '?')
            {
                map[i][j] = aux[row + i][column + j];
                priority[i][j] = aux_prio[row + i][column + j];
            }
                
        }
    }
}

void ComportamientoJugador::rotateMap(const Sensores & sensores, const State & st, vector<vector<unsigned char>> & map)
{
    int size = map.size();
    int rotations = sensores.sentido - current_state.brujula;

    for (int k = 0; k < rotations; k++){
        // Transpose matrix
        for (int i = 0; i < size-1; i++)
        {
            for (int j = i+1; j < size; j++)
            {
                swap(map[i][j], map[j][i]);
            }
        }
        // Vertical mirror
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size/2; j++)
            {
                swap(map[i][j], map[i][size-1-j]);
            }
        } 
    }
}

void ComportamientoJugador::printCliffs()
{
    int size = mapaResultado.size();
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < size; j++)
        {
            mapaResultado[i][j] = 'P';
            mapaResultado[j][i] = 'P';
            mapaResultado[size-1-i][j] = 'P';
            mapaResultado[j][size-1-i] = 'P';
        }
    }
}

void ComportamientoJugador::printSensors(const Sensores & sensores)
{
    // Muestra el valor de los sensores
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
	for (int i = 0; i < sensores.terreno.size(); i++)
		cout << sensores.terreno[i];

	cout << "  Agentes: ";
	for (int i = 0; i < sensores.agentes.size(); i++)
		cout << sensores.agentes[i];

	cout << "\nColision: " << sensores.colision;
	cout << "  Reset: " << sensores.reset;
	cout << "  Vida: " << sensores.vida;
    cout << "  Bateria: " << sensores.bateria;
    cout << "\nBikini: " << bikini;
    cout << "  Zapatillas: " << zapatillas;
    cout << "  Objetivo: " << objective.fil << " " << objective.col << endl << endl;
}

void ComportamientoJugador::resetState()
{
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
    aux_map.clear();
    aux_prio.clear();
    aux_map.resize(200, vector<unsigned char>(200, '?'));
    aux_prio.resize(200, vector<unsigned int>(200, 0));
}

int ComportamientoJugador::interact(Action accion, int valor)
{
	return false;
}
