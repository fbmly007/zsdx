/*
 * Copyright (C) 2009 Christopho, Zelda Solarus - http://www.zelda-solarus.com
 * 
 * Zelda: Mystery of Solarus DX is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Zelda: Mystery of Solarus DX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "MapLoader.h"
#include "Map.h"
#include "MapScript.h"
#include "FileTools.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "movements/FallingHeight.h"
#include "entities/EntityType.h"
#include "entities/Obstacle.h"
#include "entities/Layer.h"
#include "entities/MapEntities.h"
#include "entities/Tileset.h"
#include "entities/Tile.h"
#include "entities/Teletransporter.h"
#include "entities/DestinationPoint.h"
#include "entities/PickableItem.h"
#include "entities/DestructibleItem.h"
#include "entities/Chest.h"
#include "entities/JumpSensor.h"
#include "entities/Enemy.h"
#include "entities/InteractiveEntity.h"
#include "entities/Block.h"
#include "entities/DynamicTile.h"
#include "entities/Switch.h"
#include "entities/CustomObstacle.h"
#include "entities/Sensor.h"
#include "entities/CrystalSwitch.h"
#include "entities/CrystalSwitchBlock.h"
#include "entities/ShopItem.h"
#include "entities/ConveyorBelt.h"
#include "entities/Door.h"
#include <iomanip>
using namespace std;

/**
 * Creates a map loader.
 */
MapLoader::MapLoader(void) {

}

/**
 * Destructor.
 */
MapLoader::~MapLoader(void) {

}

/**
 * Loads a map.
 * @param map the map to load
 */
void MapLoader::load_map(Map *map) {

  // get the id of the map
  int id = map->get_id();

  // compute the file name, depending on the id
  std::ostringstream oss;
  oss << "maps/map" << std::setfill('0') << std::setw(4) << id << ".zsd";
  const string &file_name = FileTools::data_file_add_prefix(oss.str());

  // open the map file
  std::ifstream map_file(file_name.c_str());
  if (!map_file) {
    DIE("Cannot load map '" << id << "': unable to open map file '" << file_name << "'");
  }

  // parse the map file
  string line;
  TilesetId tileset_id;
  int x, y, width, height;

  // first line: map general info
  // syntax: width height world floor x y small_keys_variable tileset_id music_id
  if (!std::getline(map_file, line)) {
    DIE("Cannot load map '" << id << "': the file '" << file_name << "' is empty");
  }

  std::istringstream iss0(line);
  FileTools::read(iss0, width);
  FileTools::read(iss0, height);
  FileTools::read(iss0, map->world);
  FileTools::read(iss0, map->floor);
  FileTools::read(iss0, x);
  FileTools::read(iss0, y);
  FileTools::read(iss0, map->small_keys_variable);
  FileTools::read(iss0, tileset_id);
  FileTools::read(iss0, map->music_id);

  map->location.w = width;
  map->location.h = height;
  map->width8 = map->location.w / 8;
  map->height8 = map->location.h / 8;
  map->location.x = x;
  map->location.y = y;

  map->tileset = ResourceManager::get_tileset(tileset_id);
  if (!map->tileset->is_loaded()) {
    map->tileset->load();
  }

  // create the lists of entities and initialize obstacle_tiles
  MapEntities *entities = map->get_entities();
  entities->map_width8 = map->width8;
  entities->map_height8 = map->height8;
  entities->obstacle_tiles_size = map->width8 * map->height8;
  for (int layer = 0; layer < LAYER_NB; layer++) {

    entities->obstacle_tiles[layer] = new Obstacle[entities->obstacle_tiles_size];
    for (int i = 0; i < entities->obstacle_tiles_size; i++) {
      entities->obstacle_tiles[layer][i] = OBSTACLE_NONE;
    }
  }
  entities->boomerang = NULL;

  // read the entities
  while (std::getline(map_file, line)) {

    int entity_type, layer;

    std::istringstream iss(line);
    FileTools::read(iss, entity_type);
    FileTools::read(iss, layer);
    FileTools::read(iss, x);
    FileTools::read(iss, y);

    MapEntity *e;
    switch (entity_type) {

      case TILE:                   e = Tile::create_from_stream(iss, Layer(layer), x, y);                break;
      case DESTINATION_POINT:      e = DestinationPoint::create_from_stream(iss, Layer(layer), x, y);    break;
      case TELETRANSPORTER:        e = Teletransporter::create_from_stream(iss, Layer(layer), x, y);     break;
      case PICKABLE_ITEM:          e = PickableItem::create_from_stream(iss, Layer(layer), x, y);        break;
      case DESTRUCTIBLE_ITEM:      e = DestructibleItem::create_from_stream(iss, Layer(layer), x, y);    break;
      case CHEST:                  e = Chest::create_from_stream(iss, Layer(layer), x, y);               break;
      case JUMP_SENSOR:            e = JumpSensor::create_from_stream(iss, Layer(layer), x, y);          break;
      case ENEMY:                  e = Enemy::create_from_stream(iss, Layer(layer), x, y);               break;
      case INTERACTIVE_ENTITY:     e = InteractiveEntity::create_from_stream(iss, Layer(layer), x, y);   break;
      case BLOCK:                  e = Block::create_from_stream(iss, Layer(layer), x, y);               break;
      case DYNAMIC_TILE:           e = DynamicTile::create_from_stream(iss, Layer(layer), x, y);         break;
      case SWITCH:                 e = Switch::create_from_stream(iss, Layer(layer), x, y);              break;
      case CUSTOM_OBSTACLE:        e = CustomObstacle::create_from_stream(iss, Layer(layer), x, y);      break;
      case SENSOR:                 e = Sensor::create_from_stream(iss, Layer(layer), x, y);              break;
      case CRYSTAL_SWITCH:         e = CrystalSwitch::create_from_stream(iss, Layer(layer), x, y);       break;
      case CRYSTAL_SWITCH_BLOCK:   e = CrystalSwitchBlock::create_from_stream(iss, Layer(layer), x, y);  break;
      case SHOP_ITEM:              e = ShopItem::create_from_stream(iss, Layer(layer), x, y);            break;
      case CONVEYOR_BELT:          e = ConveyorBelt::create_from_stream(iss, Layer(layer), x, y);        break;
      case DOOR:                   e = Door::create_from_stream(iss, Layer(layer), x, y);                break;

      default:
        DIE("Error while loading map '" << id << "': unknown entity type '" << entity_type << "'");
    }
    entities->add_entity(e);
  }

  // load the script
  map->script = new MapScript(map);
  map->camera = new Camera(map);
}

