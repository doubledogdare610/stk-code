//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "config/player.hpp"
#include "config/user_config.hpp"
#include "kart_selection.hpp"
#include "graphics/irr_driver.hpp"
#include "gui/widget.hpp"
#include "gui/engine.hpp"
#include "gui/screen.hpp"
#include "gui/state_manager.hpp"
#include "input/input.hpp"
#include "input/input_manager.hpp"
#include "input/device_manager.hpp"
#include "input/input_device.hpp"
#include "karts/kart.hpp"
#include "karts/kart_properties_manager.hpp"
#include "utils/translation.hpp"
#include "utils/string_utils.hpp"

#include <string>

InputDevice* player_1_device = NULL;

using namespace GUIEngine;

namespace StateManager
{
    class PlayerKartWidget;
    
    // ref only since we're adding them to a Screen, and the Screen will take ownership of these widgets
    ptr_vector<PlayerKartWidget, REF> g_player_karts;
    
    class PlayerKartWidget : public Widget
    {
    public:
        LabelWidget* playerIDLabel;
        SpinnerWidget* playerName;
        ModelViewWidget* modelView;
        LabelWidget* kartName;
        
        int player_id_x, player_id_y, player_id_w, player_id_h;
        int player_name_x, player_name_y, player_name_w, player_name_h;
        int model_x, model_y, model_w, model_h;
        int kart_name_x, kart_name_y, kart_name_w, kart_name_h;

        int target_x, target_y, target_w, target_h;
        
        PlayerKartWidget(Widget* area, const int playerID) : Widget()
        {
            // FIXME : if a player removes itself, all IDs need to be updated
            this->m_properties[PROP_ID] = StringUtils::insert_values("@p%i", playerID);
            
            setSize(area->x, area->y, area->w, area->h);
            target_x = x;
            target_y = y;
            target_w = w;
            target_h = h;
            
            playerIDLabel = new LabelWidget();
            playerIDLabel->m_properties[PROP_TEXT] = StringUtils::insert_values(_("Player %i (keyboard)"), playerID); // TODO : determine this string dynamically
            playerIDLabel->m_properties[PROP_TEXT_ALIGN] = "center";
            playerIDLabel->m_properties[PROP_ID] = StringUtils::insert_values("@p%i_label", playerID);
            playerIDLabel->x = player_id_x;
            playerIDLabel->y = player_id_y;
            playerIDLabel->w = player_id_w;
            playerIDLabel->h = player_id_h;
            //playerID->setParent(this);
            m_children.push_back(playerIDLabel);
            
            playerName = new SpinnerWidget();
            playerName->x = player_name_x;
            playerName->y = player_name_y;
            playerName->w = player_name_w;
            playerName->h = player_name_h;
            
            const int playerAmount = UserConfigParams::m_player.size();
            playerName->m_properties[PROP_MIN_VALUE] = "0";
            playerName->m_properties[PROP_MAX_VALUE] = (playerAmount-1);
            playerName->m_properties[PROP_ID] = StringUtils::insert_values("@p%i_spinner", playerID);
            //playerName->setParent(this);
            m_children.push_back(playerName);
            
            
            modelView = new ModelViewWidget();
            
            modelView->x = model_x;
            modelView->y = model_y;
            modelView->w = model_w;
            modelView->h = model_h;
            modelView->m_properties[PROP_ID] = StringUtils::insert_values("@p%i_model", playerID);
            //modelView->setParent(this);
            m_children.push_back(modelView);
            
            // Init kart model
            KartModel* kartModel = kart_properties_manager->getKart("tux")->getKartModel();
            
            this->modelView->addModel( kartModel->getModel() );
            this->modelView->addModel( kartModel->getWheelModel(0), kartModel->getWheelGraphicsPosition(0) );
            this->modelView->addModel( kartModel->getWheelModel(1), kartModel->getWheelGraphicsPosition(1) );
            this->modelView->addModel( kartModel->getWheelModel(2), kartModel->getWheelGraphicsPosition(2) );
            this->modelView->addModel( kartModel->getWheelModel(3), kartModel->getWheelGraphicsPosition(3) );
            
            kartName = new LabelWidget();
            kartName->m_properties[PROP_TEXT] = _("Tux");
            kartName->m_properties[PROP_TEXT_ALIGN] = "center";
            kartName->m_properties[PROP_ID] = StringUtils::insert_values("@p%i_kartname", playerID);
            kartName->x = kart_name_x;
            kartName->y = kart_name_y;
            kartName->w = kart_name_w;
            kartName->h = kart_name_h;
            //kartName->setParent(this);
            m_children.push_back(kartName);
        }
        virtual void add()
        {
            playerIDLabel->add();
            playerName->add();
            modelView->add();
            kartName->add();
            
            modelView->update(0);
            
            // TODO : only fill list on first add
            const int playerAmount = UserConfigParams::m_player.size();
            for(int n=0; n<playerAmount; n++)
            {
                playerName->addLabel( UserConfigParams::m_player[n].getName() );
            }
            
        }
        void move(const int x, const int y, const int w, const int h)
        {
            target_x = x;
            target_y = y;
            target_w = w;
            target_h = h;
        }
        
        void onUpdate(float delta)
        {
            if (target_x == x && target_y == y && target_w == w && target_h == h) return;
            
            int move_step = (int)(delta*1000.0f);
            
            // move x towards target
            if (x < target_x)
            {
                x += move_step;
                if (x > target_x) x = target_x; // don't move to the other side of the target
            }
            else if (x > target_x)
            {
                x -= move_step;
                if (x < target_x) x = target_x; // don't move to the other side of the target
            }
            
            // move y towards target
            if (y < target_y)
            {
                y += move_step;
                if (y > target_y) y = target_y; // don't move to the other side of the target
            }
            else if (y > target_y)
            {
                y -= move_step;
                if (y < target_y) y = target_y; // don't move to the other side of the target
            }
            
            // move w towards target
            if (w < target_w)
            {
                w += move_step;
                if (w > target_w) w = target_w; // don't move to the other side of the target
            }
            else if (w > target_w)
            {
                w -= move_step;
                if (w < target_w) w = target_w; // don't move to the other side of the target
            }
            // move h towards target
            if (h < target_h)
            {
                h += move_step;
                if (h > target_h) h = target_h; // don't move to the other side of the target
            }
            else if (h > target_h)
            {
                h -= move_step;
                if (h < target_h) h = target_h; // don't move to the other side of the target
            }
            
            setSize(x, y, w, h);
            
            playerIDLabel->move(player_id_x,
                                player_id_y,
                                player_id_w,
                                player_id_h);
            playerName->move(player_name_x,
                             player_name_y,
                             player_name_w,
                             player_name_h );
            modelView->move(model_x,
                            model_y,
                            model_w,
                            model_h);
            kartName->move(kart_name_x,
                           kart_name_y,
                           kart_name_w,
                           kart_name_h);
            
        }
        
        void setSize(const int x, const int y, const int w, const int h)
        {
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
            
            player_id_x = x;
            player_id_y = y;
            player_id_w = w;
            player_id_h = 25;
            
            player_name_w = std::min(400, w);
            player_name_x = x + w/2 - player_name_w/2;
            player_name_y = y + 25;
            player_name_h = 40;
            
            const int modelMaxHeight =  h - 25 - 65;
            const int modelMaxWidth =  w;
            const int bestSize = std::min(modelMaxWidth, modelMaxHeight);
            const int modelY = y + 65;
            model_x = x + w/2 - bestSize*1.2/2;
            model_y = modelY + modelMaxHeight/2 - bestSize/2;
            model_w = bestSize*1.2; // FIXME : for some reason, it looks better this way, though full square should be ok
            model_h = bestSize;
            
            kart_name_x = x;
            kart_name_y = y + h - 25;;
            kart_name_w = w;
            kart_name_h = 25;
        }
    };
    

class KartHoverListener : public RibbonGridHoverListener
    {
    public:
        void onSelectionChanged(RibbonGridWidget* theWidget, const std::string& selectionID)
        {
            //std::cout << "hovered " << selectionID.c_str() << std::endl;
            
            if(selectionID.size() == 0) return;
            
            // TODO : support players other than player 0 (i.e. multiplayer)
            ModelViewWidget* w3 = g_player_karts[0].modelView;
            assert( w3 != NULL );
            
            const KartProperties* kart = kart_properties_manager->getKart(selectionID);
            if(kart == NULL) return;
            KartModel* kartModel = kart->getKartModel();
            
            w3->clearModels();
            w3->addModel( kartModel->getModel() );
            w3->addModel( kartModel->getWheelModel(0), kartModel->getWheelGraphicsPosition(0) );
            w3->addModel( kartModel->getWheelModel(1), kartModel->getWheelGraphicsPosition(1) );
            w3->addModel( kartModel->getWheelModel(2), kartModel->getWheelGraphicsPosition(2) );
            w3->addModel( kartModel->getWheelModel(3), kartModel->getWheelGraphicsPosition(3) );
            w3->update(0);
            
            // TODO : support players other than player 0 (i.e. multiplayer)
            g_player_karts[0].kartName->setText( kart->getName().c_str() );
        }
    };
KartHoverListener* karthoverListener = NULL;

void firePressedOnNewDevice(InputDevice* device)
{
    std::cout << "===== firePressedOnNewDevice =====\n";

    if(device == NULL)
    {
        std::cout << "I don't know which device was pressed :'(\n";
        return;
    }
    else if(device->getType() == DT_KEYBOARD)
    {
        std::cout << "Fire was pressed on a keyboard\n";
    }
    else if(device->getType() == DT_GAMEPAD)
    {
        std::cout << "Fire was pressed on a gamepad\n";
    }

    // make a copy of the area, ands move it to be outside the screen
    Widget rightarea = *getCurrentScreen()->getWidget("playerskarts");
    rightarea.x = irr_driver->getFrameSize().Width;
    
    // FIXME : player ID needs to be synced with active player list
    PlayerKartWidget* newPlayer = new PlayerKartWidget(&rightarea, g_player_karts.size());
    getCurrentScreen()->manualAddWidget(newPlayer);
    g_player_karts.push_back(newPlayer);
    newPlayer->add();
    
    const int amount = g_player_karts.size();
    
    Widget* fullarea = getCurrentScreen()->getWidget("playerskarts");
    const int splitWidth = fullarea->w / amount;
    
    for (int n=0; n<amount; n++)
    {
        g_player_karts[n].move( fullarea->x + splitWidth*n, fullarea->y, splitWidth, fullarea->h );
    }
    
    
}
    
void setPlayer0Device(InputDevice* device)
{
    std::cout << "===== setPlayer0Device =====\n";
    
    if(device == NULL)
    {
        std::cout << "I don't know which device to assign to player 0 :'(\n";
        return;
    }
    else if(device->getType() == DT_KEYBOARD)
    {
        std::cout << "Player 0 is using a keyboard\n";
    }
    else if(device->getType() == DT_GAMEPAD)
    {
        std::cout << "Player 0 is using a gamepad\n";
    }
    
    // TODO : support more than 1 player
    ActivePlayer* newPlayer = new ActivePlayer(UserConfigParams::m_player.get(0));
    StateManager::addActivePlayer( newPlayer );
    newPlayer->setDevice(device);
    
    input_manager->getDeviceList()->setAssignMode(DETECT_NEW);
    
    // TODO : fall back in no-assign mode when aborting a game and going back to the menu
    // how to revert assign mode :
    // StateManager::resetActivePlayers();
    // input_manager->getDeviceList()->setNoAssignMode(true);
}
    
    
void kartSelectionUpdate(float delta)
{
    const int amount = g_player_karts.size();
    for (int n=0; n<amount; n++)
    {
        g_player_karts[n].onUpdate(delta);
    }
}
    
/**
 * Callback handling events from the kart selection menu
 */
void menuEventKarts(Widget* widget, std::string& name)
{
    if(name == "init")
    {
        g_player_karts.clearWithoutDeleting();      
        
        RibbonGridWidget* w = getCurrentScreen()->getWidget<RibbonGridWidget>("karts");
        assert( w != NULL );
        
        if(karthoverListener == NULL)
        {
            karthoverListener = new KartHoverListener();
            w->registerHoverListener(karthoverListener);
        }
        
        if(!getCurrentScreen()->m_inited)
        {

            Widget* area = getCurrentScreen()->getWidget("playerskarts");
            
            // Build kart list
            const int kart_amount = kart_properties_manager->getNumberOfKarts();
            for(int n=0; n<kart_amount; n++)
            {
                const KartProperties* prop = kart_properties_manager->getKartById(n);
                std::string icon_path = "karts/";
                icon_path += prop->getIdent() + "/" + prop->getIconFile();
                w->addItem(prop->getName().c_str(), prop->getIdent().c_str(), icon_path.c_str());
                
            }
            
            PlayerKartWidget* playerKart1 = new PlayerKartWidget(area, 0 /* first player */);
            getCurrentScreen()->manualAddWidget(playerKart1);
            playerKart1->add();
            g_player_karts.push_back(playerKart1);

            
            getCurrentScreen()->m_inited = true;
        }
        w->updateItemDisplay();
        
        
        getCurrentScreen()->m_inited = true;
    } // end if init
    else if(name == "karts")
    {
        RibbonGridWidget* w = getCurrentScreen()->getWidget<RibbonGridWidget>("karts");
        assert( w != NULL );
        
        g_player_karts.clearWithoutDeleting();      
        race_manager->setLocalKartInfo(0, w->getSelectionIDString());
        
        input_manager->getDeviceList()->setAssignMode(ASSIGN);

        StateManager::pushMenu("racesetup.stkgui");
    }
}

}
