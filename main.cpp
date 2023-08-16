/*
автор: Лев Слободеник
описание: Функция поиска самого дешевого маршрута. Функция должна получать на входе три параметра: название населенного пункта отправления, название населенного пункта прибытия, а также список, каждый элемент которого представляет собой названия неких двух населенных пунктов и стоимость проезда от одного населенного пункта до другого. На выходе функция должна возвращать самый дешевый маршрут между населенными пунктами отправления и прибытия, в виде списка транзитных населенных пунктов (в порядке следования), а также общую стоимость проезда
*/

#include <iostream>
#include <algorithm>
#include <list>
#include <map>
#include <sstream>
#include <thread>

//-------------------------------------

struct RouteItem
{
    std::string start_point;
    std::string end_point;
    int         price;
};

//-------------------------------------

struct GraphItem
{
    std::string                 nodeName;
    int                         weight       = -1;
    int                         weightParent = -1;
    bool                        isPassed     = false;
    GraphItem *                 parentNode   = NULL;
    std::list< GraphItem* >     neighbours;

    void AddNeighbour( GraphItem* g, int wgt )
    {
        g->weight = wgt;
        g->parentNode = this;
        g->weightParent = weight;
        neighbours.push_back( g );
    }

    int GetWight( GraphItem * g )
    {
        std::list< GraphItem* >::iterator git = find_if( neighbours.begin(), neighbours.end(), [g] (const GraphItem* gi) { return gi->nodeName == g->nodeName; } );
        if( git != neighbours.end() )
            return (*git)->weight;
        return -1;
    }

    bool SetWeight( int w, GraphItem * parentGraph )
    {
        if( weight <= 0 || w < weight )
        {
            weight = w;
            weightParent = parentGraph->weight;
            parentNode = parentGraph;
            return true;
        }
        return false;
    }
};

//-------------------------------------

struct Graph
{
    std::list< GraphItem* > nodes;

    void Add( GraphItem * g )
    {
        std::list< GraphItem* >::iterator git =
                find_if( nodes.begin(), nodes.end(), [g] (const GraphItem* gi) { return (gi->nodeName == g->nodeName) && (gi->weight == g->weight); } );
        if( !(git != nodes.end()) )
            nodes.push_back( g );
    }

    std::list< GraphItem* > GetNode( std::string n )
    {
        std::list< GraphItem* > out;
        std::list< GraphItem* >::iterator git = find_if( nodes.begin(), nodes.end(), [n] (const GraphItem* g) { return g->nodeName == n; } );
        while( git != nodes.end() )
        {
            out.push_back(*git);
            ++git;
            git = find_if( git, nodes.end(), [n] (const GraphItem* g) { return g->nodeName == n; } );
        }
        return out;
    }
};

//-------------------------------------

typedef std::list< GraphItem* > Path;

struct PathCalculator
{
    Graph *             graph;
    GraphItem *         start_node;
    GraphItem *         end_node;
    std::list< Path >   routes;

    void CalcWeights( GraphItem * node )
    {
        node->neighbours.sort( [] (const GraphItem * a, const GraphItem * b) { return a->weight < b->weight; } );
        std::list< GraphItem* >::iterator git = node->neighbours.begin();
        for( ; git != node->neighbours.end(); ++git )
        {
            GraphItem * g = (*git);
            int new_wieght = node->weight + g->weight;
            std::list< Path >::iterator pit = routes.begin();
            for( ; pit != routes.end(); ++pit )
            {
                Path path = (*pit);
                if( path.back()->nodeName == node->nodeName )
                    g->SetWeight( new_wieght, node );
            }
        }
        node->isPassed = true;

        git = node->neighbours.begin();
        for( ; git != node->neighbours.end(); ++git )
        {
            GraphItem * g = (*git);
            if( !g->isPassed )
                CalcWeights( g );
        }
    }

    Path GetCheapestWay()
    {
        Path path;
        GraphItem * finish = end_node;
        while( finish != start_node )
        {
            path.push_front( finish );
            finish = finish->parentNode;
        }
        path.push_front( start_node );
        return path;
    }


    void CalcOneRoute( GraphItem * finish_node )
    {
        end_node = finish_node;
        CalcWeights( start_node );
        Path p = GetCheapestWay();
        routes.push_back( p );

        Path::iterator it = p.begin();
        int w = 0;
        std::stringstream ss;
        ss << "Calculate in the thread : Route : ";
        for(; it != p.end(); ++it )
        {
            w += (*it)->weight;
            ss << (*it)->nodeName << " ";
        }
        ss << "\t Price : " << w;
        std::cout << ss.str() << std::endl;
    }

    void CalcOneRouteInThread( GraphItem * finish_node )
    {
        std::thread t( &PathCalculator::CalcOneRoute, this, finish_node );
        t.join();
    }
};

//-------------------------------------

std::pair< int, std::string > LookUpOptimalRoute( std::string start_point, std::string end_point, std::list< RouteItem* > routes )
{
    Graph * graph = new Graph();

    for( std::list< RouteItem* >::iterator rit = routes.begin(); rit != routes.end(); ++rit )
    {
        GraphItem * g_from = NULL;
        GraphItem * g_to   = NULL;
        RouteItem * ri = (*rit);

        std::list< GraphItem* >::iterator gnit;
        std::list< GraphItem* > start_list = graph->GetNode( ri->start_point );
        if( start_list.size() > 1 )
        {
            std::list< GraphItem* >::iterator it = start_list.begin();
            for( ; it != start_list.end(); ++it )
            {
                g_from = (*it);
                gnit = find_if( graph->nodes.begin(), graph->nodes.end(), [ri] (const GraphItem* gi) { return (gi->nodeName == ri->end_point) && (gi->weight == ri->price); } );
                if( gnit != graph->nodes.end() )
                {
                    g_to = (*gnit);
                }
                else
                {
                    g_to = new GraphItem();
                    g_to->nodeName = ri->end_point;
                    graph->Add( g_to );
                }
                g_from->AddNeighbour( g_to, ri->price );
            }
            continue;
        }

        gnit = find_if( graph->nodes.begin(), graph->nodes.end(), [ri] (const GraphItem* gi) { return (gi->nodeName == ri->start_point) /*&& (gi->weightParent == -1)*/; } );
        if( gnit != graph->nodes.end() )
        {
            g_from = (*gnit);
        }
        else
        {
            g_from = new GraphItem();
            g_from->nodeName = ri->start_point;
            graph->Add( g_from );
        }
        gnit = find_if( graph->nodes.begin(), graph->nodes.end(), [ri] (const GraphItem* gi) { return (gi->nodeName == ri->end_point) && (gi->weight == ri->price); } );
        if( gnit != graph->nodes.end() )
        {
            g_to = (*gnit);
        }
        else
        {
            g_to = new GraphItem();
            g_to->nodeName = ri->end_point;
            graph->Add( g_to );
        }
        g_from->AddNeighbour( g_to, ri->price );
    }

    PathCalculator * pathCalc = new PathCalculator();
    pathCalc->graph = graph;
    std::list< GraphItem* > start_nodes_list = graph->GetNode( start_point ); //избыточно : точка отправления всегда одна, в листе всегда только один элемент
    std::list< GraphItem* > end_nodes_list = graph->GetNode( end_point );

    std::list< GraphItem* >::iterator start_it = start_nodes_list.begin();
//    for( ; start_it != start_nodes_list.end(); ++start_it )
//    {
        pathCalc->start_node = (*start_it);
        pathCalc->start_node->weight = 0;
        pathCalc->start_node->weightParent = 0;
        std::list< GraphItem* >::iterator end_it = end_nodes_list.begin();
        for( ; end_it != end_nodes_list.end(); ++end_it )
        {
            pathCalc->CalcOneRouteInThread( (*end_it) );
        }
//    }

    std::list< Path >::iterator pit = pathCalc->routes.begin();
    std::map< int, std::string > pmap;
    for( ; pit != pathCalc->routes.end(); ++pit )
    {
        std::list< GraphItem* > lst = (*pit);
        Path::iterator it = lst.begin();
        int w = 0;
        std::stringstream ss;
        ss << "Cheapest Path : ";
        for(; it != lst.end(); ++it )
        {
            w += (*it)->weight;
            ss << (*it)->nodeName << " ";
        }
        pmap.insert( std::make_pair( w, ss.str() ) );
    }

    return (*pmap.begin());
}

//-------------------------------------


int main(int argc, char *argv[])
{
    std::cout << "+++++++++++++++" << std::endl;

    std::list< RouteItem* > routes;

    RouteItem * r = new RouteItem();
    r->price = 7;
    r->start_point = "A";
    r->end_point = "B";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 2;
    r->start_point = "A";
    r->end_point = "C";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 8;
    r->start_point = "A";
    r->end_point = "D";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 3;
    r->start_point = "C";
    r->end_point = "B";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 5;
    r->start_point = "B";
    r->end_point = "E";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 6;
    r->start_point = "C";
    r->end_point = "D";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 4;
    r->start_point = "D";
    r->end_point = "E";
    routes.push_back( r );

    r = new RouteItem();
    r->price = 11;
    r->start_point = "C";
    r->end_point = "E";
    routes.push_back( r );

    //++++++++++++++++++++++++++


    std::pair< int, std::string > p = LookUpOptimalRoute( "A", "E", routes );

    std::cout << p.second << std::endl;
    std::cout << "Price : " << p.first << std::endl;

    std::cout << "+++++++++++++++" << std::endl;


    return 0;
}
