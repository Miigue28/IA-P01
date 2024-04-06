#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>

using namespace std;

Action ComportamientoJugador::think(Sensores sensores)
{
	Action action = actIDLE;

	//printSensors(sensores);

    Movement move;

    if (sensores.colision)
    {
        last_action = actIDLE;
    }

    if (sensores.reset)
    {
        resetState();
    }

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

    switch (sensores.nivel)
    {
        case 0:
            current_state.fil = sensores.posF;
            current_state.col = sensores.posC;
            current_state.brujula = sensores.sentido;
            bien_situado = true;
        break;
        case 3:
            faulty = true;
        break;
    }

    if (bien_situado)
    {
        printMap(sensores.terreno, current_state, mapaResultado, priority, faulty);
        printPriorities(sensores.terreno, current_state, priority, faulty);
    }
    else
    {
        printMap(sensores.terreno, current_state, aux_map, aux_prio, faulty);
        printPriorities(sensores.terreno, current_state, aux_prio, faulty);
    }

    switch (sensores.terreno[0])
    {
        case 'G':
            if (!bien_situado)
            {
                translateMap(sensores, current_state, aux_map, mapaResultado);
                rotateMap(sensores, current_state, mapaResultado);
                current_state.fil = sensores.posF;
                current_state.col = sensores.posC;
                current_state.brujula = sensores.sentido;
                bien_situado = true;
            }
        break;
        case 'X':
            need_reload = false;
            reload = true;
            // Nos esperamos hasta que recarguemos suficientemente la batería
            if (!desperate && sensores.bateria < 4500)
            {
                last_action = action;
                return action;
            }
        break;
        case 'K':
            if (bien_situado && !bikini) modifyPriority(mapaResultado, priority); else modifyPriority(aux_map, aux_prio);
            bikini = true;
        break;
        case 'D':
            if (bien_situado && !zapatillas) modifyPriority(mapaResultado, priority); else modifyPriority(aux_map, aux_prio);
            zapatillas = true;
        break;
    }

    // Si nos queda poco tiempo de vida evitamos pararnos a recargar
    if (sensores.vida < HEALTH_THRESH)
    {
        desperate = true;
    }

    // Si estamos escasos de batería vamos a la casilla de recarga más cercana
    // if (!desperate && sensores.bateria < BATTERY_THRESH)
    // {
    //     // Siempre que hayamos visto previamente una casilla de recarga
    //     if (reload && !goto_objective)
    //     {
    //         objective = searchSquare(bien_situado ? mapaResultado : aux_map, 'X');
    //         if (measureDistance((Square) current_state, objective) < 15)
    //         {
    //             goto_objective = true;
    //             need_reload = true;
    //         }
    //     }
    // }

    // Si llevamos mucho tiempo en la misma zona buscamos la casilla más cercana sin explorar
    // si está relativamente cerca nuestra vamos a ella, si no nos movemos aleatoriamente
    if (!random && detectCicle(bien_situado ? priority : aux_prio))
    {
        random = true;
        //if (!goto_objective)
        //{
        //    objective = searchSquare(bien_situado ? mapaResultado : aux_map, '?');
        //    if (measureDistance((Square) current_state, objective) < 25)
        //    {
        //        goto_objective = true;
        //    }
        //    else
        //    {
        //        random = true;
        //    }
        //    
        //}
    }

    // Si nos chocamos contra un muro activamos el protocolo para gestionarlos
    if (sensores.terreno[2] == 'M')
    {
        wall_protocol = true;
    }

    // Cuando nos encontramos muy cerca de nuestro objetivo lo damos por encontrado
    if (goto_objective && measureDistance((Square) current_state, objective) < 2)
    {
        goto_objective = false;
    }

    // Fase de decisión de la nueva acción
    if (!protocol.empty())
    {
        action = protocol.front();
        protocol.pop_front();
    }
    else if (random)
    {
        action = randomlyMove(sensores.terreno, sensores.agentes);
    }
    else if (goto_objective)
    {
        action = goToObjective(sensores.terreno, sensores.agentes);
    }
    else if (wall_protocol)
    {
        action = wallProtocol(sensores.terreno, sensores.agentes);
    }
    else if (!bien_situado && detectPositioning(sensores.terreno))
    {
        action = goToSquare(sensores.terreno, sensores.agentes, 'G');
    }
    else if (!desperate && detectReload(sensores.terreno) && sensores.bateria < 4000)
    {
        action = goToSquare(sensores.terreno, sensores.agentes, 'X');
    }
    else if (!bikini && detectBikini(sensores.terreno))
    {
        action = goToSquare(sensores.terreno, sensores.agentes, 'K');
    }
    else if (!zapatillas && detectZapatillas(sensores.terreno))
    {
        action = goToSquare(sensores.terreno, sensores.agentes, 'D'); 
    }
    else
    {
        action = selectMovement(sensores.terreno, sensores.agentes, bien_situado ? priority : aux_prio);
    }
    
    // Devuelve el valor de la acción
    last_action = action;
	return action;
}

Action ComportamientoJugador::randomlyMove(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    cont_random++;
    Action action = actIDLE;
    if (accesibleSquare(terreno, agentes, 2) && cont_actWALK < 15)
    {
        cont_actWALK++;
        action = actWALK;
    }
    else
    {
        cont_actWALK = 0;
        action = randomlyRotate();
    }
    if (cont_random > 25)
    {
        cont_random = 0;
        random = false;
    }
    return action;
}

Action ComportamientoJugador::randomlyRotate()
{
    return (rand() % 2 == 0) ? actTURN_SR : actTURN_L;
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
        {
            reload = true;
            return true;
        }
    }
    return false;
}

bool ComportamientoJugador::detectCicle(const vector<vector<unsigned int>> & prio)
{
    int row = current_state.fil, column = current_state.col;
    int row_lowerbound = row, row_upperbound = row;
    int column_lowerbound = column, column_upperbound = column;
    int size = prio.size(); 
    int mean = prio[row][column];
    int count = 1;
    for (int k = 1; k < 3; k++)
    {
        row_lowerbound = (3 <= row - k ? row - k : row_lowerbound);
        column_lowerbound = (3 <= column - k ? column - k : column_lowerbound);
        row_upperbound = (row + k < size - 3 ? row + k : row_upperbound);
        column_upperbound = (column + k < size - 3 ? column + k : column_upperbound);
        
        for (int i = column_lowerbound; i <= column_upperbound; i++)
        {
            if (prio[row_lowerbound][i] < 500)
            {
                mean += prio[row_lowerbound][i];
                count++;
            }
            if (prio[row_upperbound][i] < 500)
            {
                mean += prio[row_upperbound][i];
                count++;
            }
        }
        for (int i = row_lowerbound+1; i <= row_upperbound-1; i++)
        {
            if (prio[i][column_lowerbound] < 500)
            {
                mean += prio[i][column_lowerbound];
                count++;
            }
            if (prio[i][column_upperbound] < 500)
            {
                mean += prio[i][column_upperbound];
                count++;
            }
        }
    }

    if (bikini && zapatillas)
    {
        return mean/count > 35;
    }
    else
    {
        return mean/count > 75;
    }
}

Action ComportamientoJugador::goToSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, unsigned char square)
{
    // TODO: HACER MÁS EFICIENTE ESTÁ BUSQUEDA
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
            return (accesibleSquare(terreno, agentes, 2) && accesibleSquare(terreno, agentes, 6) ? actRUN : actTURN_SR);
        else if (index < 6)
            return (accesibleSquare(terreno, agentes, 2) ? actWALK : actTURN_L);
        else
            return actTURN_SR;
    }
    else
    {
        if (index <= 12)
            return (accesibleSquare(terreno, agentes, 2) && accesibleSquare(terreno, agentes, 6) ? actRUN : actTURN_L);
        else
            return actTURN_SR;
    }
}

Action ComportamientoJugador::goToObjective(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    int row = current_state.fil;
    int column = current_state.col;
    Movement move_forward = setMovement(current_state.brujula, straight);
    Movement move_right = setMovement(current_state.brujula, rightdiagonal);
    Movement move_left = setMovement(current_state.brujula, leftdiagonal);
    Action action = actTURN_L;

    // Calculamos las distancias al objetivo
    unsigned int min = setDistance(terreno[0], row, column);
    unsigned int min_left = setDistance(terreno[1], row + move_left.fil, column + move_left.col);
    unsigned int min_forward = setDistance(terreno[2], row + move_forward.fil, column + move_forward.col);
    unsigned int min_right = setDistance(terreno[3], row + move_right.fil, column + move_right.col);
    
    // Elegimos el movimiento más conveniente priorizando siempre avanzar
    if (min_right < min)
    {
        min = min_right;
        action = actTURN_SR;
    }
    if (min_left < min)
    {
        min = min_left;
        action = actTURN_L;
    }
    if (min_forward <= min)
    {
        action = actWALK;
    }
    return action;
}

Action ComportamientoJugador::wallProtocol(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes)
{
    if (!accesibleSquare(terreno, agentes, 1) && !accesibleSquare(terreno, agentes, 2) && !accesibleSquare(terreno, agentes, 3))
        return randomlyRotate();
    else if (!accesibleSquare(terreno, agentes, 1) && !accesibleSquare(terreno, agentes, 2))
        return actTURN_SR;
    else if (!accesibleSquare(terreno, agentes, 2) && !accesibleSquare(terreno, agentes, 3))
        return actTURN_L;
    else if (!accesibleSquare(terreno, agentes, 1) && !accesibleSquare(terreno, agentes, 3))
        return accesibleSquare(terreno, agentes, 2) ? actWALK : actTURN_L;
    else if (!accesibleSquare(terreno, agentes, 1) || !accesibleSquare(terreno, agentes, 3))
    {
        had_walls = (terreno[1] == 'M' ? leftdiagonal : rightdiagonal);
        return accesibleSquare(terreno, agentes, 2) ? actWALK : actTURN_L;
    }
    else
    {
        wall_protocol = false;
        switch (had_walls)
        {
            case rightdiagonal:
                if (accesibleSquare(terreno, agentes, 3))
                {
                    protocol.emplace_back(actWALK);
                    return actTURN_SR;
                }
                return actTURN_L;
            case leftdiagonal:
                if (accesibleSquare(terreno, agentes, 2))
                {
                    protocol.emplace_back(actTURN_L);
                    // protocol.emplace_back(actWALK);
                    return actWALK;
                }
                return actTURN_L;
            default:
                return randomlyRotate();
        }
    }
}

int ComportamientoJugador::measureDistance(const Square & st1, const Square & st2)
{
    return max(abs(st1.fil - st2.fil),  abs(st1.col - st2.col));
}

int ComportamientoJugador::setDistance(const unsigned char & sq, int i, int j)
{
    if (goto_objective)
    {
        if (sq == 'P' || sq == 'M')
            return 500;
        else
            return measureDistance(objective, {i, j});
    }
}

void ComportamientoJugador::setPrioritySearch(const vector<vector<unsigned char>> & map, vector<vector<unsigned int>> & dist, const Square & objective)
{
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size(); j++)
        {
            if (map[i][j] != '?')
            {
                if (map[i][j] == 'P' || map[i][j] == 'M')
                    dist[i][j] = 500;
                else
                    dist[i][j] = measureDistance(objective, {i, j});
            }
        }
    }       
}

int ComportamientoJugador::setPriority(const unsigned char & sq)
{
    switch (sq)
    {
        case 'A': return bikini ? 5 : 50; break;
        case 'B': return zapatillas ? 5 : 50; break;
        case 'P': return 500; break;
        case 'M': return 500; break;
        case 'T': return 5; break;
        case 'S': return 5; break;
        case 'G': return 5; break;
        case 'K': return 5; break;
        case 'D': return 5; break;
        case 'X': return 5; break;
        //default: return 0; break;
    }
}

void ComportamientoJugador::modifyPriority(const vector<vector<unsigned char>> & map, vector<vector<unsigned int>> & prio)
{
    for (int i = 0; i < map.size(); i++)
    {
        for (int j = 0; j < map[i].size(); j++)
        {
            if (bikini && map[i][j] == 'A')
                prio[i][j] -= 45;

            if (zapatillas && map[i][j] == 'B')
                prio[i][j] -= 45;
        }
    }
}

Action ComportamientoJugador::selectMovement(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, const vector<vector<unsigned int>> & prio)
{
    int walk = 2, run = 6;
    unsigned int min = 500, min_leftdiag = 500, min_rightdiag = 500;
    Movement move = setMovement(current_state.brujula, straight);
    Action action = actTURN_L;
    if (accesibleSquare(terreno, agentes, walk))
    {
        min = prio[current_state.fil + move.fil][current_state.col + move.col];
        action = actWALK;
        if (!faulty && accesibleSquare(terreno, agentes, run) && convenientSquare(terreno, agentes, run))
        {
            unsigned int run_prio = prio[current_state.fil + 2*move.fil][current_state.col + 2*move.col];
            if (run_prio < min)
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

bool ComportamientoJugador::canMoveDiagonally(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, const vector<vector<unsigned int>> & prio, Vision vision, unsigned int & min)
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
        if (!faulty && accesibleSquare(terreno, agentes, run))
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

bool ComportamientoJugador::convenientSquare(const vector<unsigned char> & terreno, const vector<unsigned char> & agentes, int index)
{
    if (terreno[index] == 'A' && !bikini)
        return false;
    else if (terreno[index] == 'B' && !zapatillas)
        return false;
    else
        return true;
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

Square ComportamientoJugador::searchSquare(const vector<vector<unsigned char>> & map, char square)
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
            if (map[i][column_lowerbound] == square)
            {
                tmp = {i, column_lowerbound};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
            if (map[i][column_upperbound] == square)
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
            if (map[row_lowerbound][i] == square)
            {
                tmp = {row_lowerbound, i};
                distance = measureDistance(tmp, base);
                if (distance < min)
                {
                    out = tmp;
                    min = distance;
                }
            }
            if (map[row_upperbound][i] == square)
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

void ComportamientoJugador::printMap(const vector<unsigned char> & terreno, const State & st, vector<vector<unsigned char>> & map, vector<vector<unsigned int>> & prio, bool faulty)
{
    int k = 0;
    if (map[st.fil][st.col] == '?')
    {
        map[st.fil][st.col] = terreno[k];
        prio[st.fil][st.col] = setPriority(terreno[k]);
    }
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

void ComportamientoJugador::printPriorities(const vector<unsigned char> & terreno, const State & st, vector<vector<unsigned int>> & prio, bool faulty)
{
    int k = 1;
    prio[st.fil][st.col] += 5;
    switch (st.brujula)
    {
        case norte:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                        continue;

                    prio[st.fil - i][st.col - i - 1 + j]++;
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
                        continue;

                    if (j < i + 1)
                        prio[st.fil - i][st.col + j - 1]++;
                    else
                        prio[st.fil - i - i - 1 + j][st.col + i]++;

                    k++;
                }
            }     
        break;
        case este:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                        continue;


                    prio[st.fil - i - 1 + j][st.col + i]++;
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
                continue;

            if (j < i + 1)
                prio[st.fil + j - 1][st.col + i]++;
            else
                prio[st.fil + i][st.col + i + i + 1 - j]++;

            k++;
          }
        }    
        break;
        case sur:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                        continue;
                    
                    prio[st.fil + i][st.col + i + 1 - j]++;
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
                    continue;

                if (j < i + 1)
                    prio[st.fil + i][st.col - j + 1]++;

                else
                    prio[st.fil + i + i + 1 - j][st.col - i]++;

                k++;
              }
            }    
        break;
        case oeste:
            for (int i = 1; i < 4; i++)
            {
                for (int j = 1; j < 2*i+2; j++)
                {
                    if (faulty && (k == 6 || k == 11 || k == 12 || k == 13))
                        continue;

                    prio[st.fil + i + 1 - j][st.col - i]++;
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
                continue;

            if (j < i + 1)
                prio[st.fil - j + 1][st.col - i]++;
            else
                prio[st.fil - i][st.col - i - i - 1 + j]++;
            k++;
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
    reload = false;
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