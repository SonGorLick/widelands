/*
 * Copyright (C) 2009-2017 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "ai/ai_help_structs.h"

#include "base/macros.h"
#include "base/time_string.h"
#include "logic/map.h"
#include "logic/map.h"
#include "logic/player.h"

// couple of constants for calculation of road interconnections
constexpr int kRoadNotFound = -1000;
constexpr int kShortcutWithinSameEconomy = 1000;
constexpr int kRoadToDifferentEconomy = 10000;
constexpr int kUpperDefaultMutationLimit = 100;
constexpr int kLowerDefaultMutationLimit = 40;

namespace Widelands {

// CheckStepRoadAI
CheckStepRoadAI::CheckStepRoadAI(Player* const pl, uint8_t const mc, bool const oe)
   : player(pl), movecaps(mc), open_end(oe) {
}

bool CheckStepRoadAI::allowed(
   Map& map, FCoords start, FCoords end, int32_t, CheckStep::StepId const id) const {
	uint8_t endcaps = player->get_buildcaps(end);

	// we should not cross fields with road or flags (or any other immovable)
	if ((map.get_immovable(start)) && !(id == CheckStep::stepFirst)) {
		return false;
	}

	// Calculate cost and passability
	if (!(endcaps & movecaps))
		return false;

	// Check for blocking immovables
	if (BaseImmovable const* const imm = map.get_immovable(end))
		if (imm->get_size() >= BaseImmovable::SMALL) {
			if (id != CheckStep::stepLast && !open_end)
				return false;

			if (dynamic_cast<Flag const*>(imm))
				return true;

			if (!dynamic_cast<Road const*>(imm) || !(endcaps & BUILDCAPS_FLAG))
				return false;
		}

	return true;
}

bool CheckStepRoadAI::reachable_dest(const Map& map, const FCoords& dest) const {
	NodeCaps const caps = dest.field->nodecaps();

	if (!(caps & movecaps)) {
		if (!((movecaps & MOVECAPS_SWIM) && (caps & MOVECAPS_WALK)))
			return false;

		if (!map.can_reach_by_water(dest))
			return false;
	}

	return true;
}

// We are looking for fields we can walk on
// and owned by hostile player.
FindNodeEnemy::FindNodeEnemy(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeEnemy::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && fc.field->get_owned_by() != 0 &&
	       player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// We are looking for buildings owned by hostile player
// (sometimes there is a enemy's teritorry without buildings, and
// this confuses the AI)
FindNodeEnemiesBuilding::FindNodeEnemiesBuilding(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeEnemiesBuilding::accept(const Map&, const FCoords& fc) const {
	return (fc.field->get_immovable()) && fc.field->get_owned_by() != 0 &&
	       player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// When looking for unowned terrain to acquire, we are actually
// only interested in fields we can walk on.
// Fields should either be completely unowned or owned by an opposing player
FindEnemyNodeWalkable::FindEnemyNodeWalkable(Player* p, Game& g)
   : player(p), game(g) {
}

bool FindEnemyNodeWalkable::accept(const Map&, const FCoords& fc) const {
	return ((fc.field->nodecaps() & MOVECAPS_WALK) &&
	       (fc.field->get_owned_by() > 0) &&
	        player->is_hostile(*game.get_player(fc.field->get_owned_by())));
}

// Sometimes we need to know how many nodes our allies owns
FindNodeAllyOwned::FindNodeAllyOwned(Player* p, Game& g, PlayerNumber n)
   : player(p), game(g), player_number(n) {
}

bool FindNodeAllyOwned::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && (fc.field->get_owned_by() != 0) &&
	       (fc.field->get_owned_by() != player_number) &&
	       !player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// When looking for unowned terrain to acquire, we must
// pay speciall attention to fields where mines can be built.
// Fields should be completely unowned
FindNodeUnownedMineable::FindNodeUnownedMineable(Player* p, Game& g, int32_t t) : player(p), game(g), ore_type(t) {
}

bool FindNodeUnownedMineable::accept(const Map&, const FCoords& fc) const {
	if (ore_type == INVALID_INDEX) {
		return (fc.field->nodecaps() & BUILDCAPS_MINE)  && (fc.field->get_owned_by() == 0);
	}
	return (fc.field->nodecaps() & BUILDCAPS_MINE)  && (fc.field->get_owned_by() == 0) && fc.field->get_resources() == ore_type;
}

FindNodeUnownedBuildable::FindNodeUnownedBuildable(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeUnownedBuildable::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & BUILDCAPS_SIZEMASK) && (fc.field->get_owned_by() == 0);
}

// Unowned but walkable fields nearby
FindNodeUnownedWalkable::FindNodeUnownedWalkable(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeUnownedWalkable::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && (fc.field->get_owned_by() == 0);
}

// Looking only for mines-capable fields nearby
// of specific type
FindNodeMineable::FindNodeMineable(Game& g, DescriptionIndex r) : game(g), res(r) {
}

bool FindNodeMineable::accept(const Map&, const FCoords& fc) const {

	return (fc.field->nodecaps() & BUILDCAPS_MINE) && (fc.field->get_resources() == res);
}

// Fishers and fishbreeders must be built near water
FindNodeWater::FindNodeWater(const World& world) : world_(world) {
}

bool FindNodeWater::accept(const Map& map, const FCoords& coord) const {
	return (world_.terrain_descr(coord.field->terrain_d()).get_is() &
	        TerrainDescription::Is::kWater) ||
	       (world_.terrain_descr(map.get_neighbour(coord, WALK_W).field->terrain_r()).get_is() &
	        TerrainDescription::Is::kWater) ||
	       (world_.terrain_descr(map.get_neighbour(coord, WALK_NW).field->terrain_r()).get_is() &
	        TerrainDescription::Is::kWater);
}

bool FindNodeOpenWater::accept(const Map& /* map */, const FCoords& coord) const {
	return !(coord.field->nodecaps() & MOVECAPS_WALK) && (coord.field->nodecaps() & MOVECAPS_SWIM);
}

// FindNodeWithFlagOrRoad
bool FindNodeWithFlagOrRoad::accept(const Map&, FCoords fc) const {
	if (upcast(PlayerImmovable const, pimm, fc.field->get_immovable()))
		return (dynamic_cast<Flag const*>(pimm) ||
		        (dynamic_cast<Road const*>(pimm) && (fc.field->nodecaps() & BUILDCAPS_FLAG)));
	return false;
}

NearFlag::NearFlag(const Flag& f, int32_t const c, int32_t const d)
   : flag(&f), cost(c), distance(d) {
}

EventTimeQueue::EventTimeQueue()
     {}

void EventTimeQueue::push(const uint32_t production_time){
	queue.push(production_time);
}

uint32_t EventTimeQueue::count(const uint32_t current_time){
	strip_old(current_time);
	return queue.size();
}

void EventTimeQueue::strip_old(const uint32_t current_time){
	while (queue.size() > 0 && queue.front() < current_time - duration_) {
		queue.pop();
	}
}

BuildableField::BuildableField(const Widelands::FCoords& fc)
	: coords(fc),
	  field_info_expiration(20000),
	  preferred(false),
	  enemy_nearby(0),
	  enemy_accessible_(false),
	  unowned_land_nearby(0),
	  near_border(false),
	  unowned_mines_spots_nearby(0),
	  unowned_iron_mines_nearby(false),
	  trees_nearby(0),
	  // explanation of starting values
	  // this is done to save some work for AI (CPU utilization)
	  // base rules are:
	  // count of rocks can only decrease, so  amount of rocks
	  // is recalculated only when previous count is positive
	  // count of water fields are stable, so if the current count is
	  // non-negative, water is not recaldulated
	  rocks_nearby(1),
	  water_nearby(-1),
	  open_water_nearby(-1),
	  distant_water(0),
	  fish_nearby(-1),
	  critters_nearby(-1),
	  ground_water(1),
	  space_consumers_nearby(0),
	  rangers_nearby(0),
	  area_military_capacity(0),
	  military_loneliness(1000),
	  military_in_constr_nearby(0),
	  own_military_presence(0),
	  enemy_military_presence(0),
	  ally_military_presence(0),
	  military_stationed(0),
	  unconnected_nearby(false),
	  military_unstationed(0),
	  own_non_military_nearby(0),
	  is_portspace(Widelands::ExtendedBool::kUnset),
	  port_nearby(false),
	  portspace_nearby(Widelands::ExtendedBool::kUnset),
	  max_buildcap_nearby(0),
	  last_resources_check_time(0),
	  military_score_(0),
	  inland(false),
	  local_soldier_capacity(0),
      is_militarysite(false) {
}

int32_t BuildableField::own_military_sites_nearby_() {
	return military_stationed + military_unstationed;
}

MineableField::MineableField(const Widelands::FCoords& fc)
   : coords(fc),
     field_info_expiration(20000),
     preferred(false),
     mines_nearby(0),
     same_mine_fields_nearby(0) {
}

EconomyObserver::EconomyObserver(Widelands::Economy& e) : economy(e) {
	dismantle_grace_time = std::numeric_limits<int32_t>::max();
}

int32_t BuildingObserver::total_count() const {
	return cnt_built + cnt_under_construction;
}

bool BuildingObserver::is(BuildingAttribute attribute) const{
	return is_what.count(attribute);
	}

void BuildingObserver::set_is(const BuildingAttribute attribute) {
	is_what.insert(attribute);
	}

void BuildingObserver::unset_is(const BuildingAttribute attribute) {
	is_what.erase(attribute);
	assert(is_what.count(attribute) == 0);
	}

Widelands::AiModeBuildings BuildingObserver::aimode_limit_status() {
	if (total_count() > cnt_limit_by_aimode) {
		return Widelands::AiModeBuildings::kLimitExceeded;
	} else if (total_count() == cnt_limit_by_aimode) {
		return Widelands::AiModeBuildings::kOnLimit;
	} else {
		return Widelands::AiModeBuildings::kAnotherAllowed;
	}
}
bool BuildingObserver::buildable(Widelands::Player& p) {
	return is_what.count(BuildingAttribute::kBuildable) && p.is_building_type_allowed(id);
}

// Computer player does not get notification messages about enemy militarysites
// and warehouses, so following is collected based on observation
// It is conventient to have some information preserved, like nearby minefields,
// when it was attacked, whether it is warehouse and so on
// Also AI test more such targets when considering attack and calculated score is
// is stored in the observer
EnemySiteObserver::EnemySiteObserver()
   : is_warehouse(false),
     attack_soldiers_strength(0),
     defenders_strength(0),
     stationed_soldiers(0),
     last_time_seen(0),
     last_tested(0),
     score(0),
     mines_nearby(Widelands::ExtendedBool::kUnset),
     last_time_attacked(0),
     attack_counter(0) {
}

// as all mines have 3 levels, AI does not know total count of mines per mined material
// so this observer will be used for this
MineTypesObserver::MineTypesObserver() : in_construction(0), finished(0) {
}

ExpansionType::ExpansionType() {
	type = ExpansionMode::kResources;
}

void ExpansionType::set_expantion_type(const ExpansionMode etype) {
	type = etype;
}

//int16_t ExpansionType::get_proportions(const ScoreBlock block, bool dismantlement) {
	//if (dismantlement) {
		//return proportion_table[static_cast<uint8_t>(ScoreBlock::kDismantle)][static_cast<uint8_t>(block)];
	//}
	//return proportion_table[static_cast<uint8_t>(type)][static_cast<uint8_t>(block)];
//}



ManagementData::ManagementData() {
		score = 1;
		review_count = 0;
		primary_parent = 255;
		next_neuron_id = 0;
		performance_change = 0;
}



Neuron::Neuron(int8_t w, uint8_t f, uint16_t i) : 
	weight(w),type(f), id(i) {
	assert(type < neuron_curves.size());
	assert(weight >= -100 && weight <= 100);
	lowest_pos = std::numeric_limits<uint8_t>::max();
	highest_pos = std::numeric_limits<uint8_t>::min();
	recalculate();
}


FNeuron::FNeuron(uint32_t c, uint16_t i){
	core = c;
	id = i;
}

// Returning result depending on combinations of 5 bools
bool FNeuron::get_result(const bool bool1, const bool bool2, const bool bool3, const bool bool4, const bool bool5){
	return core.test(bool1 * 16 + bool2 * 8 + bool3 * 4 + bool4 * 2 + bool5);
}

// Returning bool on a position
bool FNeuron::get_position(const uint8_t pos) {
	assert (pos < f_neuron_bit_size);
	return core.test(pos);
}

// Returning numerical value of FNeuron. Used for saving and priting into log
uint32_t FNeuron::get_int(){
	return core.to_ulong();
}

void FNeuron::flip_bit(const uint8_t pos){
	assert (pos < f_neuron_bit_size);
	core.flip(pos);
}

// Normal neuron is defined by weight and curve
void Neuron::set_weight(int8_t w) {
	if (w > 100) {
		weight = 100;
	}else if (w < -100) {
		weight = -100;
	} else {
		weight = w;
	}
}

// Neuron stores calculated values in array
void Neuron::recalculate() {
	assert (neuron_curves.size() > type);
	for (int8_t i = 0; i <= 20; i+=1) {
		results[i] = weight * neuron_curves[type][i] / 100;
	}
}

// Following two functions returns Neuron value on position
int8_t Neuron::get_result(const uint8_t pos){
	assert(pos <= 20);
	return results[pos];
}

int8_t Neuron::get_result_safe(int32_t pos, const bool absolute){
	if (pos > highest_pos) {highest_pos = pos;};
	if (pos < lowest_pos) {lowest_pos = pos;};
	if (pos < 0) {
		pos = 0;
	}
	if (pos > 20) {
		pos = 20;
	}
	assert(pos <= 20);
	assert (results[pos] >= -100 && results[pos] <=100);
	if (absolute) {
		return std::abs(results[pos]);
	}	
	return results[pos];
}

// Setting the type of function
void Neuron::set_type(uint8_t new_type) {
	assert (new_type < neuron_curves.size());
	type = new_type;
}

// Shifting the value in range -100 to 100, if zero_align is true, it is now allowed to shift
// from negative to positive and vice versa, 0 must be used.
int8_t ManagementData::shift_weight_value(const int8_t old_value, const bool aggressive){
	
	int16_t halfVArRange = 50;
	if (aggressive){
		//printf ("aggressive mode \n");
		halfVArRange = 200;
	}
	
	const int16_t upper_limit = std::min<int16_t>(old_value + halfVArRange, 100);
	const int16_t bottom_limit = std::max<int16_t>(old_value - halfVArRange, -100);	
 	int16_t new_value = bottom_limit + std::rand() % (upper_limit - bottom_limit + 1);
	
	if (!aggressive && ((old_value > 0 && new_value < 0) || (old_value < 0 && new_value > 0))) {
		new_value = 0;
	}
	
	if (new_value < -100) {
		new_value = -100;
	}
	if (new_value > 100) {
		new_value = 100;
	}
	return static_cast<int8_t>(new_value);
}


// Used to score performance of AI
void ManagementData::review(const uint32_t gametime, PlayerNumber pn,
 const uint32_t land, const uint32_t max_e_land, const uint32_t old_land, const uint16_t attackers,
   const int16_t trained_soldiers, const int16_t latest_attackers, const uint32_t big_spots) {
	const int16_t current_land_divider = 2;
	const int16_t land_delta_multiplier = 1;
	const int16_t bonus = 1000;
	const int16_t attackers_multiplicator = 1;
	const int16_t attack_bonus = 100;
	const int16_t trained_soldiers_score = 150;	
	//assert(ironmines_bonus <= ironmines_max_bonus);
	
	const int16_t main_bonus = ((static_cast<int32_t>(land-old_land) > 0 &&
		land > max_e_land * 5 / 6 &&
		attackers > 0 &&
		trained_soldiers > 0 &&
		latest_attackers > 0) ? bonus : 0);

	const int16_t land_delta_bonus = static_cast<int16_t>(land - old_land) * land_delta_multiplier;
	
	score = land / current_land_divider
		+ land_delta_bonus
		+ main_bonus
		+ attackers * attackers_multiplicator
		+ ((attackers > 0) ? attack_bonus : -attack_bonus)
		+ trained_soldiers * trained_soldiers_score;
		
	printf (" %2d %s: reviewing AI mngm. data, sc: %4d Pr.p: %d (l: %4d / %4d / %4d, at:%4d(%3d),ts:%3d(%1d), Bigsp:%3d)\n",
		pn, gamestring_with_leading_zeros(gametime), score,
		primary_parent, 
		land / current_land_divider,
		main_bonus,
		land_delta_bonus,
		attackers * attackers_multiplicator,
		latest_attackers,
		trained_soldiers * trained_soldiers_score,
		trained_soldiers,big_spots);

	
	if (score < -10000 || score > 20000){
		printf ("MASTERERROR %2d %s: reviewing AI mngm. data, score: %4d (land: %4d  attackers: %3d)\n",
	pn, gamestring_with_leading_zeros(gametime), score,
	land / 86, attackers);
	}
	assert (score > -10000 && score < 100000);

	review_count += 1;	
}

void ManagementData::new_dna_for_persistent( const uint8_t pn, const Widelands::AiType type){

	ai_type = type;
	
	printf (" %2d ... initialize starts\n", pn);

   //AutoSCore_AIDNA_1
    const std::vector<int16_t> AI_initial_military_numbers_A =
      {  18 ,  -12 ,   31 ,   59 ,  -44 ,  -93 ,  -56 ,   81 ,   39 ,   10 ,  //AutoContent_01_AIDNA_1
        -55 ,   40 ,  -96 ,  100 ,   70 ,  -59 ,   33 ,   38 ,    0 ,  -61 ,  //AutoContent_02_AIDNA_1
         89 ,  -16 ,   29 ,   53 ,  -54 ,  -51 ,   26 ,    0 ,   36 ,    9 ,  //AutoContent_03_AIDNA_1
         34 ,    0 ,  -27 ,  -50 ,  -94 ,   19 ,  -63 ,   -7 ,   11 ,  -63 ,  //AutoContent_04_AIDNA_1
         78 ,   51 ,   -1 ,  -33 ,   59 ,   61 ,   44 ,    0 ,  -89 ,  -73 ,  //AutoContent_05_AIDNA_1
        -35 ,   -5 ,   35 ,    0 ,   64 ,  -83 ,    4 ,  -87 ,   38 ,  -92 ,  //AutoContent_06_AIDNA_1
        -87 ,   56 ,   22 ,  -33 ,  -69 ,   24 ,  -51 ,   47 ,   -4 ,  -51 ,  //AutoContent_07_AIDNA_1
        -45 ,   15 ,  -55 ,  -39 ,    0 ,  -32 ,  -98 ,  -35 ,  -41 ,  -16 ,  //AutoContent_08_AIDNA_1
         68 ,  -77 ,  -63 ,   24 ,   71 ,   79 ,    0 ,   94 ,  -60 ,   59 ,  //AutoContent_09_AIDNA_1
          0 ,   53 ,   74 ,    0 ,  -15 ,   31 ,   16 ,   19 ,    0 ,  -32 ,  //AutoContent_10_AIDNA_1
         53 ,  -45 ,   19 ,  -54 ,  -75 ,   83 ,  -78 ,  -57 ,   64 ,  -52 ,  //AutoContent_11_AIDNA_1
         69 ,  -42 ,    0 ,  -50 ,   54 ,   70 ,   48 ,   46 ,  -87 ,   43 ,  //AutoContent_12_AIDNA_1
         85 ,    2 ,    8 ,   51 ,  -80 ,    0 ,  -13 ,   71 ,   59 ,    7 ,  //AutoContent_13_AIDNA_1
        -52 ,   64 ,   26 ,  -96 ,    0 ,  -68 ,  -57 ,   75 ,   64 ,  -67 ,  //AutoContent_14_AIDNA_1
          0 ,   15 ,  -67 ,   56 ,  -57 ,   11 ,  -33 ,  -30 ,   45 ,  -100  //AutoContent_15_AIDNA_1
       }
		;
	
	assert(magic_numbers_size == AI_initial_military_numbers_A.size());
	
	const std::vector<int8_t> input_weights_A =
		//0		1		2		3	4		5		6		7		8	9
      {   48 ,  -60 ,  -18 ,   11 ,  -65 ,  -100 ,  -71 ,    0 ,   97 ,  -12 ,  //AutoContent_16_AIDNA_1
         98 ,  -59 ,  -92 ,   62 ,   81 ,   63 ,  -47 ,  -98 ,  -59 ,  -15 ,  //AutoContent_17_AIDNA_1
         -6 ,  -13 ,   59 ,  -65 ,   17 ,   29 ,   47 ,  -60 ,   83 ,   23 ,  //AutoContent_18_AIDNA_1
         16 ,    0 ,  -52 ,   28 ,  -38 ,  -50 ,  -60 ,  -42 ,  -48 ,  -69 ,  //AutoContent_19_AIDNA_1
        -70 ,  -88 ,  -66 ,  -92 ,   92 ,  -68 ,  -39 ,  -39 ,   32 ,   75 ,  //AutoContent_20_AIDNA_1
         28 ,  -40 ,   73 ,  -90 ,  -51 ,   12 ,    0 ,  -94 ,    0 ,   95 ,  //AutoContent_21_AIDNA_1
        -32 ,   29 ,   67 ,  -58 ,  -71 ,  -45 ,  -53 ,   52 ,    0 ,  -47 ,  //AutoContent_22_AIDNA_1
        -44 ,  -28 ,  -44 ,   41 ,   41 ,   17 ,   49 ,   -8 ,   36 ,  -11  //AutoContent_23_AIDNA_1
	}
			;
	const std::vector<int8_t> input_func_A =
      {    1 ,    0 ,    1 ,    2 ,    2 ,    2 ,    0 ,    0 ,    0 ,    0 ,  //AutoContent_24_AIDNA_1
          0 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    1 ,    2 ,    1 ,  //AutoContent_25_AIDNA_1
          2 ,    2 ,    1 ,    2 ,    2 ,    2 ,    0 ,    2 ,    0 ,    2 ,  //AutoContent_26_AIDNA_1
          1 ,    1 ,    0 ,    1 ,    2 ,    1 ,    1 ,    1 ,    2 ,    0 ,  //AutoContent_27_AIDNA_1
          2 ,    1 ,    2 ,    0 ,    0 ,    1 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_28_AIDNA_1
          1 ,    1 ,    0 ,    0 ,    0 ,    2 ,    0 ,    0 ,    1 ,    1 ,  //AutoContent_29_AIDNA_1
          0 ,    1 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_30_AIDNA_1
          0 ,    1 ,    0 ,    2 ,    1 ,    1 ,    1 ,    2 ,    1 ,    1  //AutoContent_31_AIDNA_1
	}
		;
	assert(neuron_pool_size == input_func_A.size());
	assert(neuron_pool_size == input_weights_A.size());

	const std::vector<uint32_t> f_neurons_A =
      {  1616885467 ,  1339535765 ,  2178712122 ,  3242499123 ,  999762572 ,  957703282 ,  268166213 ,  1137236653 ,  3369867713 ,  852115950 ,  //AutoContent_32_AIDNA_1
        2332688180 ,  12318675 ,  1649726684 ,  1760363406 ,  3669047589 ,  1080813791 ,  519462344 ,  2965112942 ,  609411483 ,  544410102 ,  //AutoContent_33_AIDNA_1
        57278227 ,  3219617408 ,  4014461879 ,  1004977504 ,  666020052 ,  485350693 ,  1478130029 ,  2497939662 ,  1710040215 ,  390326038 ,  //AutoContent_34_AIDNA_1
        743796723 ,  3236867374 ,  133858409 ,  4246717416 ,  3026102135 ,  3644389919 ,  4027669093 ,  1068487498 ,  3412069980 ,  3886876527 ,  //AutoContent_35_AIDNA_1
        2309748111 ,  870938591 ,  2063282097 ,  3624073273 ,  3635775655 ,  3953146608 ,  1806904467 ,  1187324532 ,  418456867 ,  3950071901 ,  //AutoContent_36_AIDNA_1
        1291543049 ,  2042605612 ,  1327419950 ,  3136682336 ,  701092635 ,  328321849 ,  3537267396 ,  3070708841 ,  2550847968 ,  3262329266  //AutoContent_37_AIDNA_1
	 };
	assert(f_neuron_pool_size == f_neurons_A.size());

		
    //AutoSCore_AIDNA_2
	const std::vector<int16_t> AI_initial_military_numbers_B =
      {  18 ,    0 ,   31 ,   59 ,  -44 ,  -93 ,  -56 ,   81 ,   39 ,   10 ,  //AutoContent_01_AIDNA_2
        -55 ,   40 ,  -96 ,  100 ,   70 ,  -59 ,   33 ,   38 ,    0 ,  -26 ,  //AutoContent_02_AIDNA_2
         89 ,  -16 ,   29 ,   29 ,  -36 ,  -51 ,    0 ,    0 ,   36 ,    9 ,  //AutoContent_03_AIDNA_2
         34 ,    0 ,  -27 ,  -50 ,  -94 ,   19 ,  -63 ,   -7 ,   11 ,  -63 ,  //AutoContent_04_AIDNA_2
         39 ,   51 ,   -1 ,  -33 ,   59 ,   61 ,   44 ,    0 ,  -89 ,  -73 ,  //AutoContent_05_AIDNA_2
        -35 ,   -5 ,   70 ,    0 ,   64 ,  -83 ,    4 ,  -87 ,   38 ,  -92 ,  //AutoContent_06_AIDNA_2
        -87 ,   56 ,   22 ,  -33 ,  -69 ,   24 ,  -51 ,   47 ,   -4 ,  -51 ,  //AutoContent_07_AIDNA_2
        -62 ,   15 ,  -55 ,  -39 ,    0 ,  -32 ,  -81 ,  -35 ,  -41 ,  -16 ,  //AutoContent_08_AIDNA_2
         68 ,  -77 ,  -63 ,   24 ,   71 ,   79 ,    0 ,   94 ,  -60 ,   59 ,  //AutoContent_09_AIDNA_2
          0 ,   53 ,   74 ,    0 ,  -15 ,   31 ,   16 ,   19 ,    0 ,  -32 ,  //AutoContent_10_AIDNA_2
         53 ,  -45 ,   19 ,  -54 ,  -75 ,   83 ,  -78 ,  -57 ,   64 ,   -6 ,  //AutoContent_11_AIDNA_2
         69 ,  -42 ,    0 ,  -70 ,   54 ,   70 ,   48 ,   46 ,  -80 ,   43 ,  //AutoContent_12_AIDNA_2
         85 ,    2 ,   58 ,   51 ,  -80 ,    0 ,  -13 ,   71 ,   59 ,    7 ,  //AutoContent_13_AIDNA_2
        -52 ,   64 ,   26 ,  -96 ,    0 ,  -68 ,  -57 ,   75 ,   64 ,  -67 ,  //AutoContent_14_AIDNA_2
          0 ,   15 ,  -67 ,   56 ,  -57 ,   11 ,  -33 ,  -30 ,   45 ,  -100  //AutoContent_15_AIDNA_2
		}
		;
	assert(magic_numbers_size == AI_initial_military_numbers_B.size());
		
	const std::vector<int8_t> input_weights_B =
      {   48 ,  -60 ,  -18 ,   11 ,  -65 ,  -100 ,  -71 ,    0 ,   97 ,  -12 ,  //AutoContent_16_AIDNA_2
         98 ,  -59 ,  -89 ,   62 ,   81 ,   63 ,  -47 ,  -98 ,  -59 ,  -15 ,  //AutoContent_17_AIDNA_2
         -6 ,  -13 ,   59 ,  -65 ,   17 ,   29 ,   47 ,  -60 ,   37 ,   23 ,  //AutoContent_18_AIDNA_2
         16 ,    9 ,  -52 ,   28 ,  -38 ,  -50 ,  -60 ,  -42 ,  -21 ,  -69 ,  //AutoContent_19_AIDNA_2
        -70 ,  -88 ,  -66 ,  -92 ,   92 ,  -68 ,  -39 ,  -39 ,   32 ,   75 ,  //AutoContent_20_AIDNA_2
         28 ,  -40 ,   98 ,  -90 ,  -72 ,    0 ,    0 ,  -94 ,    0 ,   95 ,  //AutoContent_21_AIDNA_2
        -32 ,   29 ,   67 ,  -47 ,  -71 ,  -45 ,  -53 ,   52 ,    0 ,  -47 ,  //AutoContent_22_AIDNA_2
          0 ,  -28 ,  -44 ,   41 ,   41 ,   17 ,   49 ,   -8 ,   36 ,  -49  //AutoContent_23_AIDNA_2
}
	      ;
	
	const std::vector<int8_t> input_func_B = 
      {    1 ,    0 ,    1 ,    2 ,    2 ,    2 ,    0 ,    0 ,    0 ,    0 ,  //AutoContent_24_AIDNA_2
          0 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    1 ,    2 ,    1 ,  //AutoContent_25_AIDNA_2
          2 ,    2 ,    1 ,    2 ,    2 ,    1 ,    0 ,    2 ,    0 ,    2 ,  //AutoContent_26_AIDNA_2
          1 ,    1 ,    0 ,    1 ,    2 ,    1 ,    1 ,    1 ,    2 ,    0 ,  //AutoContent_27_AIDNA_2
          2 ,    1 ,    2 ,    0 ,    0 ,    1 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_28_AIDNA_2
          1 ,    1 ,    0 ,    0 ,    0 ,    2 ,    0 ,    0 ,    1 ,    1 ,  //AutoContent_29_AIDNA_2
          2 ,    1 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_30_AIDNA_2
          0 ,    1 ,    0 ,    2 ,    1 ,    1 ,    1 ,    2 ,    1 ,    1  //AutoContent_31_AIDNA_2
}
		;
		assert(neuron_pool_size == input_func_B.size());
		assert(neuron_pool_size == input_weights_B.size());

      
	const std::vector<uint32_t> f_neurons_B =
      {  543012571 ,  1339535509 ,  2178713146 ,  3242499123 ,  999762573 ,  957637746 ,  268430405 ,  1136712357 ,  3369867713 ,  818037230 ,  //AutoContent_32_AIDNA_2
        2332679988 ,  12318675 ,  1649722524 ,  1756170118 ,  3669045541 ,  1080813775 ,  519757256 ,  3032221806 ,  609411475 ,  544410102 ,  //AutoContent_33_AIDNA_2
        2204761875 ,  3219617408 ,  4014461879 ,  1004977504 ,  666020052 ,  485350693 ,  1478130029 ,  2497923278 ,  1710040215 ,  390325782 ,  //AutoContent_34_AIDNA_2
        739602419 ,  3303976254 ,  133858409 ,  4229940200 ,  3026102135 ,  2578508319 ,  4027669093 ,  1068488522 ,  3412068056 ,  3886876539 ,  //AutoContent_35_AIDNA_2
        2309747983 ,  870938591 ,  2063281969 ,  3624073273 ,  3635775655 ,  3953146609 ,  1806904467 ,  1187324532 ,  483468579 ,  3378597981 ,  //AutoContent_36_AIDNA_2
        1291543049 ,  2042605612 ,  3474903614 ,  3136813408 ,  684314907 ,  999410489 ,  3528944324 ,  3003567209 ,  2584410592 ,  3262329267  //AutoContent_37_AIDNA_2
	 };
	assert(f_neuron_pool_size == f_neurons_B.size());


    //AutoSCore_AIDNA_3
	const std::vector<int16_t> AI_initial_military_numbers_C =
      {  18 ,    0 ,   31 ,   59 ,  -44 ,  -93 ,  -56 ,   81 ,   39 ,   10 ,  //AutoContent_01_AIDNA_3
        -55 ,   40 ,  -96 ,  100 ,   70 ,  -59 ,   33 ,   38 ,    0 ,  -26 ,  //AutoContent_02_AIDNA_3
         89 ,  -16 ,   29 ,   29 ,  -54 ,  -51 ,    0 ,    0 ,   36 ,    9 ,  //AutoContent_03_AIDNA_3
         35 ,    0 ,  -27 ,  -50 ,  -94 ,   19 ,  -63 ,   -7 ,   11 ,  -63 ,  //AutoContent_04_AIDNA_3
         78 ,   51 ,  -34 ,  -33 ,   59 ,   61 ,   44 ,    0 ,  -89 ,  -73 ,  //AutoContent_05_AIDNA_3
        -35 ,   -5 ,   70 ,  -12 ,   64 ,  -83 ,    4 ,  -87 ,   38 ,  -92 ,  //AutoContent_06_AIDNA_3
        -87 ,   56 ,   22 ,  -33 ,  -69 ,   24 ,  -51 ,   47 ,   -4 ,  -51 ,  //AutoContent_07_AIDNA_3
        -62 ,   15 ,  -55 ,  -39 ,    0 ,  -32 ,  -81 ,  -35 ,  -41 ,  -16 ,  //AutoContent_08_AIDNA_3
         68 ,  -77 ,  -63 ,   24 ,   71 ,   79 ,    0 ,   94 ,  -60 ,   59 ,  //AutoContent_09_AIDNA_3
          0 ,   53 ,   77 ,    0 ,  -15 ,   31 ,   16 ,   19 ,    0 ,  -32 ,  //AutoContent_10_AIDNA_3
         53 ,  -45 ,   19 ,  -54 ,  -75 ,   83 ,  -78 ,  -57 ,   64 ,   -6 ,  //AutoContent_11_AIDNA_3
         69 ,  -42 ,    0 ,  -70 ,   66 ,   70 ,   48 ,   46 ,  -80 ,   43 ,  //AutoContent_12_AIDNA_3
         85 ,    2 ,   58 ,   51 ,  -80 ,    0 ,  -13 ,   71 ,   59 ,    7 ,  //AutoContent_13_AIDNA_3
        -52 ,   64 ,   26 ,  -96 ,    0 ,  -68 ,  -57 ,   75 ,   64 ,  -67 ,  //AutoContent_14_AIDNA_3
          0 ,   15 ,  -67 ,   56 ,  -57 ,   11 ,  -33 ,  -30 ,   45 ,  -100  //AutoContent_15_AIDNA_3
       }

		;
	
		assert(magic_numbers_size == AI_initial_military_numbers_C.size());
	
	const std::vector<int8_t> input_weights_C =
      {   48 ,  -60 ,  -18 ,   11 ,  -65 ,  -100 ,  -71 ,    0 ,   97 ,  -12 ,  //AutoContent_16_AIDNA_3
         98 ,  -59 ,  -89 ,   62 ,   81 ,   63 ,  -47 ,  -98 ,  -59 ,  -15 ,  //AutoContent_17_AIDNA_3
         -6 ,  -13 ,   59 ,  -65 ,   17 ,   29 ,   47 ,  -60 ,   37 ,   23 ,  //AutoContent_18_AIDNA_3
         16 ,    9 ,  -52 ,   28 ,  -38 ,  -50 ,  -60 ,  -42 ,  -21 ,  -69 ,  //AutoContent_19_AIDNA_3
        -70 ,  -88 ,  -66 ,  -92 ,   92 ,  -68 ,  -39 ,  -39 ,   32 ,   75 ,  //AutoContent_20_AIDNA_3
         59 ,  -40 ,   98 ,  -90 ,  -72 ,    0 ,    0 ,  -94 ,    0 ,   95 ,  //AutoContent_21_AIDNA_3
        -32 ,   29 ,   67 ,  -47 ,  -71 ,  -45 ,  -53 ,   52 ,    0 ,  -47 ,  //AutoContent_22_AIDNA_3
        -44 ,  -28 ,  -44 ,   41 ,   41 ,   17 ,   49 ,   -8 ,   36 ,  -49  //AutoContent_23_AIDNA_3
       }
			;
	const std::vector<int8_t> input_func_C =
      {    1 ,    0 ,    1 ,    2 ,    2 ,    2 ,    0 ,    0 ,    0 ,    0 ,  //AutoContent_24_AIDNA_3
          0 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    1 ,    2 ,    1 ,  //AutoContent_25_AIDNA_3
          2 ,    2 ,    1 ,    2 ,    2 ,    1 ,    2 ,    2 ,    0 ,    2 ,  //AutoContent_26_AIDNA_3
          1 ,    1 ,    0 ,    1 ,    2 ,    1 ,    1 ,    1 ,    2 ,    0 ,  //AutoContent_27_AIDNA_3
          2 ,    1 ,    2 ,    0 ,    0 ,    1 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_28_AIDNA_3
          1 ,    1 ,    0 ,    0 ,    0 ,    2 ,    0 ,    0 ,    1 ,    1 ,  //AutoContent_29_AIDNA_3
          2 ,    1 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_30_AIDNA_3
          0 ,    1 ,    0 ,    2 ,    1 ,    1 ,    1 ,    2 ,    1 ,    1  //AutoContent_31_AIDNA_3
       }
			;
	assert(neuron_pool_size == input_func_C.size());
	assert(neuron_pool_size == input_weights_C.size());
	
	const std::vector<uint32_t> f_neurons_C =
      {   6141659 ,  1339535509 ,  2161935930 ,  3242499187 ,  999762573 ,  957703282 ,  268430405 ,  1136679589 ,  3369867713 ,  818037230 ,  //AutoContent_32_AIDNA_3
        2332679988 ,  12318675 ,  1649722516 ,  1756170118 ,  3669045541 ,  1080813791 ,  519757256 ,  3032221806 ,  609411475 ,  544410102 ,  //AutoContent_33_AIDNA_3
        2200567571 ,  3219617408 ,  4014461879 ,  1004977504 ,  666020052 ,  485350693 ,  1478130029 ,  2497923278 ,  1710039191 ,  390325782 ,  //AutoContent_34_AIDNA_3
        747990005 ,  3303976222 ,  133858409 ,  4229940200 ,  3026102135 ,  2578508319 ,  4027669093 ,  1068488522 ,  3412070104 ,  3819767679 ,  //AutoContent_35_AIDNA_3
        2309747983 ,  870938591 ,  2063281969 ,  3624073273 ,  3635775655 ,  2879404784 ,  1806904723 ,  1187324004 ,  2633049379 ,  3413209181 ,  //AutoContent_36_AIDNA_3
        1291543049 ,  2042605612 ,  3407794750 ,  3136813408 ,  684314907 ,  831638329 ,  3528878788 ,  3003567209 ,  2584410592 ,  3262329267  //AutoContent_37_AIDNA_3
	 };
	assert(f_neuron_pool_size == f_neurons_C.size());

		
    //AutoSCore_AIDNA_4
	const std::vector<int16_t> AI_initial_military_numbers_D =
      {  18 ,    0 ,   31 ,   59 ,  -44 ,  -93 ,  -56 ,   81 ,   39 ,   10 ,  //AutoContent_01_AIDNA_4
        -55 ,   40 ,  -96 ,  100 ,   70 ,  -59 ,   33 ,   38 ,    0 ,  -26 ,  //AutoContent_02_AIDNA_4
         89 ,  -16 ,   29 ,   29 ,  -54 ,  -51 ,    0 ,    0 ,   36 ,    9 ,  //AutoContent_03_AIDNA_4
         34 ,    0 ,  -27 ,  -50 ,  -94 ,   19 ,  -63 ,  -22 ,   11 ,  -63 ,  //AutoContent_04_AIDNA_4
         39 ,   51 ,  -10 ,  -33 ,   59 ,   61 ,   44 ,    0 ,  -89 ,  -73 ,  //AutoContent_05_AIDNA_4
        -35 ,   -5 ,   70 ,    0 ,   64 ,  -83 ,    4 ,  -87 ,   38 ,  -92 ,  //AutoContent_06_AIDNA_4
        -87 ,   56 ,   22 ,  -33 ,  -69 ,   24 ,  -51 ,   47 ,   -4 ,  -51 ,  //AutoContent_07_AIDNA_4
        -62 ,   15 ,  -55 ,  -39 ,    0 ,  -32 ,  -81 ,  -35 ,  -41 ,  -16 ,  //AutoContent_08_AIDNA_4
         68 ,  -77 ,  -63 ,   24 ,   71 ,   79 ,    0 ,   94 ,  -60 ,   59 ,  //AutoContent_09_AIDNA_4
          0 ,   53 ,   74 ,    0 ,  -15 ,   31 ,   16 ,   19 ,    0 ,  -32 ,  //AutoContent_10_AIDNA_4
         53 ,  -45 ,   19 ,  -54 ,  -75 ,   83 ,  -78 ,  -57 ,   64 ,   -6 ,  //AutoContent_11_AIDNA_4
         69 ,  -42 ,    0 ,  -70 ,   54 ,   70 ,   48 ,   46 ,  -80 ,   43 ,  //AutoContent_12_AIDNA_4
         85 ,    2 ,   58 ,   51 ,  -80 ,    0 ,  -13 ,   71 ,   59 ,    7 ,  //AutoContent_13_AIDNA_4
        -52 ,   64 ,   26 ,  -96 ,    0 ,  -68 ,  -57 ,   75 ,   64 ,  -67 ,  //AutoContent_14_AIDNA_4
          0 ,   15 ,  -67 ,   56 ,  -57 ,   11 ,  -33 ,  -30 ,   45 ,  -100  //AutoContent_15_AIDNA_4
	}
		;
	assert(magic_numbers_size == AI_initial_military_numbers_D.size());
		
	const std::vector<int8_t> input_weights_D =
      {   48 ,  -60 ,  -18 ,   11 ,  -65 ,  -100 ,  -71 ,    0 ,   97 ,  -12 ,  //AutoContent_16_AIDNA_4
         98 ,  -59 ,  -89 ,   62 ,   81 ,   63 ,  -47 ,  -98 ,  -59 ,  -15 ,  //AutoContent_17_AIDNA_4
         -6 ,  -13 ,   59 ,  -65 ,   17 ,   29 ,   47 ,  -60 ,   37 ,   23 ,  //AutoContent_18_AIDNA_4
         16 ,    9 ,  -52 ,   28 ,  -38 ,  -50 ,  -60 ,  -42 ,  -21 ,  -69 ,  //AutoContent_19_AIDNA_4
        -70 ,  -88 ,  -66 ,  -92 ,   92 ,  -68 ,  -39 ,  -39 ,   32 ,   75 ,  //AutoContent_20_AIDNA_4
         28 ,  -40 ,   98 ,  -90 ,  -72 ,    0 ,    0 ,  -94 ,    0 ,   95 ,  //AutoContent_21_AIDNA_4
        -32 ,   29 ,   67 ,  -58 ,  -71 ,  -45 ,  -53 ,   52 ,    0 ,  -47 ,  //AutoContent_22_AIDNA_4
        -44 ,  -28 ,  -44 ,   41 ,   41 ,   17 ,   49 ,   -8 ,   36 ,  -11  //AutoContent_23_AIDNA_4
	}
	      ;
	
	const std::vector<int8_t> input_func_D = 
      {    1 ,    0 ,    1 ,    2 ,    2 ,    2 ,    0 ,    0 ,    0 ,    0 ,  //AutoContent_24_AIDNA_4
          0 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    1 ,    2 ,    1 ,  //AutoContent_25_AIDNA_4
          2 ,    2 ,    2 ,    2 ,    2 ,    1 ,    0 ,    2 ,    0 ,    2 ,  //AutoContent_26_AIDNA_4
          1 ,    1 ,    0 ,    1 ,    2 ,    1 ,    1 ,    1 ,    2 ,    0 ,  //AutoContent_27_AIDNA_4
          2 ,    1 ,    2 ,    0 ,    0 ,    1 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_28_AIDNA_4
          1 ,    1 ,    0 ,    0 ,    0 ,    2 ,    0 ,    0 ,    1 ,    1 ,  //AutoContent_29_AIDNA_4
          2 ,    1 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_30_AIDNA_4
          0 ,    1 ,    0 ,    2 ,    1 ,    1 ,    1 ,    2 ,    1 ,    1  //AutoContent_31_AIDNA_4
	}
		;
	assert(neuron_pool_size == input_func_D.size());
	assert(neuron_pool_size == input_weights_D.size());

	const std::vector<uint32_t> f_neurons_D =
      {  543012571 ,  1339535509 ,  2178778682 ,  3242499123 ,  999762573 ,  957637746 ,  268430405 ,  1136712357 ,  3369867713 ,  818037230 ,  //AutoContent_32_AIDNA_4
        2332679988 ,  12318675 ,  1112851612 ,  1756170118 ,  3669045541 ,   7071951 ,  519757256 ,  3032221806 ,  609411475 ,  544410102 ,  //AutoContent_33_AIDNA_4
        2271870739 ,  3219617408 ,  4014461879 ,  1004977504 ,  666020052 ,  485350693 ,  1478130029 ,  2497956046 ,  1710058647 ,  390325782 ,  //AutoContent_34_AIDNA_4
        739602419 ,  3303976254 ,  133858409 ,  4229940200 ,  3026102131 ,  2578508319 ,  4027669093 ,  1068488522 ,  3412070136 ,  3886876539 ,  //AutoContent_35_AIDNA_4
        2309747983 ,  870938591 ,  2063281969 ,  3624073273 ,  3635775655 ,  3953146608 ,  1806904467 ,  1187324532 ,  215033123 ,  3413200989 ,  //AutoContent_36_AIDNA_4
        1291543049 ,  2042605612 ,  3474903614 ,  3136813408 ,  684314907 ,  999410489 ,  3528878788 ,  3003567209 ,  2584410592 ,  3262329267  //AutoContent_37_AIDNA_4
	 };
	assert(f_neuron_pool_size == f_neurons_D.size());
	
	primary_parent = std::rand() % 4;
	const uint8_t parent2 = std::rand() % 4;
	
	printf (" ... DNA initialization (primary parent: %d, secondary parent: %d)\n", primary_parent, parent2);
	
	// First setting military numbers, they goes dirrectly to persistent data
	for (uint16_t i = 0; i < magic_numbers_size; i += 1){
		// Child inherites DNA with probability 5:1 from main parent
		uint8_t dna_donor = (std::rand() % 50 > 0) ? primary_parent : parent2;
		if (i == MutationRatePosition) { //overwritting
			dna_donor = primary_parent;
		}
		
		switch ( dna_donor ) {
			case 0 : 
				set_military_number_at(i,AI_initial_military_numbers_A[i]);
				break;
			case 1 : 
				set_military_number_at(i,AI_initial_military_numbers_B[i]);
				break;
			case 2 : 
				set_military_number_at(i,AI_initial_military_numbers_C[i]);
				break;
			case 3 : 
				set_military_number_at(i,AI_initial_military_numbers_D[i]);
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
			}
		}

	pd->neuron_weights.clear();
	pd->neuron_functs.clear();
	pd->f_neurons.clear();	

	for (uint16_t i = 0; i <neuron_pool_size; i += 1){ //NOCOM
		const uint8_t dna_donor = (std::rand() % 20 > 0) ? primary_parent : parent2;
		
		switch ( dna_donor ) {
			case 0 : 
				pd->neuron_weights.push_back(input_weights_A[i]);
				pd->neuron_functs.push_back(input_func_A[i]);
				break;
			case 1 : 
				pd->neuron_weights.push_back(input_weights_B[i]);
				pd->neuron_functs.push_back(input_func_B[i]);
				break;
			case 2 : 
				pd->neuron_weights.push_back(input_weights_C[i]);
				pd->neuron_functs.push_back(input_func_C[i]);
				break;
			case 3 : 
				pd->neuron_weights.push_back(input_weights_D[i]);
				pd->neuron_functs.push_back(input_func_D[i]);
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
		}
	}


	for (uint16_t i = 0; i <f_neuron_pool_size; i += 1){
		const uint8_t dna_donor = (std::rand() % 20 > 0) ? primary_parent : parent2;
		switch ( dna_donor ) {
			case 0 : 
				pd->f_neurons.push_back(f_neurons_A[i]);
				//f_neuron_pool.push_back(FNeuron(f_neurons_A[i], i));				
				break;
			case 1 : 
				pd->f_neurons.push_back(f_neurons_B[i]);
				//f_neuron_pool.push_back(FNeuron(f_neurons_B[i], i));	
				break;
			case 2 : 
				pd->f_neurons.push_back(f_neurons_C[i]);
				//f_neuron_pool.push_back(FNeuron(f_neurons_C[i], i));					
				break;
			case 3 : 
				pd->f_neurons.push_back(f_neurons_D[i]);
				//f_neuron_pool.push_back(FNeuron(f_neurons_D[i], i));	
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
		}
	}
	
	pd->magic_numbers_size = magic_numbers_size;
	pd->neuron_pool_size = neuron_pool_size;
	pd->f_neuron_pool_size = f_neuron_pool_size;

	//test_consistency(); might not be consistent yet
	
}

void ManagementData::mutate(const uint8_t pn){

	const int8_t old_probability = get_military_number_at(MutationRatePosition);

	int16_t probability = shift_weight_value(get_military_number_at(MutationRatePosition), false) + 101;
	if (probability > kUpperDefaultMutationLimit) {
			probability = kUpperDefaultMutationLimit;
	}
	if (probability < kLowerDefaultMutationLimit) {
			probability = kLowerDefaultMutationLimit;
	}

	set_military_number_at(MutationRatePosition, probability - 101);

	// decreasing probability (or rather increasing probability of mutation) if weaker player
	if (!kDisableHigherMutationForWeakerAI) { // Used for training of AI
		if (ai_type == Widelands::AiType::kWeak) {
			probability /= 2;
			printf (" Weak mode, increasing mutation probability to 1 / %d\n", probability);
		} else if (ai_type == Widelands::AiType::kVeryWeak) {
			probability /= 4;
			printf (" Very weak mode, increasing mutation probability to 1 / %d\n", probability);
		}
	}

	//Wild card
	if (std::rand() % 8 == 0) {
		probability /= 3;
		printf ("Wild card new probability: %d \n", probability);
	}
	
	assert(probability > 0 && probability <= 201 );
	
	printf (" %2d ... mutating, final prob.: 1 / %3d (old: 1 / %3d)\n",
	pn,
	probability,
	old_probability + 101);

	if (probability < 201) {
		
		// Modifying pool of Military numbers
		{
			std::set<int32_t>preferred_numbers = {};
		
			for (uint16_t i = 0; i < magic_numbers_size; i += 1){
				if (i == MutationRatePosition) { //mutated above
					continue;
				}
				bool mutating = false;
				bool aggressive = false;
				if (preferred_numbers.count(i) > 0) {
					mutating = true;
					aggressive = true;
				} else if (std::rand() % probability == 0) {
					mutating = true;
				}
			   	if (mutating) {
					const int16_t old_value = get_military_number_at(i);
					const int16_t new_value = shift_weight_value(get_military_number_at(i), aggressive);
					set_military_number_at(i,new_value);
					printf ("      Magic number %3d: value changed: %4d -> %4d  %s\n", 
						i, old_value,
						new_value,
						(aggressive) ? "aggressive" : "");
				}
			}
		}
	
		// Modifying pool of neurons
		{
			std::set<int32_t> preferred_neurons = {};
			for (auto& item : neuron_pool){
	
				bool mutating = false;
				bool aggressive = false;
				if (preferred_neurons.count(item.get_id()) > 0) {
					mutating = true;
					aggressive = true;
				} else if (std::rand() % probability == 0) {
					mutating = true;
				}			
				if (mutating) {
					const int16_t old_value = item.get_weight();
					if (std::rand() % 4 == 0){
						assert(neuron_curves.size() > 0);
						item.set_type(std::rand() % neuron_curves.size());
						pd->neuron_functs[item.get_id()] = item.get_type();
					} else {
						int16_t new_value = shift_weight_value(item.get_weight(), aggressive);
						item.set_weight(new_value);			
						pd->neuron_weights[item.get_id()] = item.get_weight();
					}
					printf ("      Neuron %2d: weight: %4d -> %4d, new curve: %d   %s\n",
					 item.get_id(), old_value, item.get_weight(), item.get_type(),
					 (aggressive) ? "aggressive" : "");
					
					item.recalculate();
				}
			}
		}
	
		// Modifying pool of f-neurons
		{
			std::set<int32_t> preferred_f_neurons = {};
			for (auto& item : f_neuron_pool){
				uint8_t changed_bits = 0;
				//is this preferred neuron
				if (preferred_f_neurons.count(item.get_id()) > 0) {
					for (uint8_t i = 0; i < f_neuron_bit_size; i += 1) {
						if (std::rand() % 5 == 0) {
							item.flip_bit(i);
							changed_bits += 1;
						}
					}					
				} else {  //normal mutation
					for (uint8_t i = 0; i < f_neuron_bit_size; i += 1) {
						if (std::rand() % (probability *3) == 0) {
							item.flip_bit(i);
							changed_bits += 1;
						}
					}
				}
					
				
				////iteating over single bits within neuron
				//bool fneuron_changed = false;
				//for (uint8_t i = 0; i < f_neuron_bit_size; i += 1) {
					//bool mutating = false;
					//if (preferred_f_neurons.count(item.get_id()) > 0 && std::rand() % (probability * 3* 10 / preferred_ratio) == 0) {
						//mutating = true;
					//} else if (std::rand() % (probability *3) == 0) {
						//mutating = true;
					//}
		
					//if (mutating) {
						//item.flip_bit(i);
						//fneuron_changed = true;
					//} 
				//}
		
				if (changed_bits) {
					pd->f_neurons[item.get_id()] = item.get_int();
					printf ("      F-Neuron %2d: new value: %13ul, changed bits: %2d   %s\n",
					item.get_id(), item.get_int(),changed_bits, (preferred_f_neurons.count(item.get_id()) > 0) ? "aggressive" : "");
				}
			}
		}
	}

	test_consistency();
}

void ManagementData::copy_persistent_to_local(const uint8_t pn) {

	assert(pd->neuron_weights.size() == neuron_pool_size);
	assert(pd->neuron_functs.size() == neuron_pool_size);
	neuron_pool.clear();
	for (uint32_t i = 0; i < neuron_pool_size; i = i+1){
		neuron_pool.push_back(Neuron(pd->neuron_weights[i],pd->neuron_functs[i],i));	
	}

	assert(pd->f_neurons.size() == f_neuron_pool_size);
	f_neuron_pool.clear();
	for (uint32_t i = 0; i < f_neuron_pool_size; i = i+1){
		//pd->f_neurons.push_back(f_neuron_pool[i].get_int());
		f_neuron_pool.push_back(FNeuron(pd->f_neurons[i], i));
	}
	
	pd->magic_numbers_size = magic_numbers_size;
	pd->neuron_pool_size = neuron_pool_size;
	pd->f_neuron_pool_size = f_neuron_pool_size;	
	
	test_consistency();
	printf (" %d: DNA initialized\n", pn);	
	
	
	}

bool ManagementData::test_consistency(bool itemized) {

	assert (pd->neuron_weights.size() == pd->neuron_pool_size);
	assert (pd->neuron_functs.size() == pd->neuron_pool_size);
	assert (neuron_pool.size() == pd->neuron_pool_size);
	assert (neuron_pool.size() == neuron_pool_size);
	
	assert (pd->magic_numbers_size == magic_numbers_size);			
	assert (pd->magic_numbers.size() == magic_numbers_size);
	
	assert (pd->f_neurons.size() == pd->f_neuron_pool_size);
	assert (f_neuron_pool.size() == pd->f_neuron_pool_size);
	assert (f_neuron_pool.size() == f_neuron_pool_size);
	
	if (itemized) {
		// comparing content of neuron and fneuron pools
		for (uint16_t i = 0; i < neuron_pool_size; i += 1){
			assert(pd->neuron_weights[i] == neuron_pool[i].get_weight());
			assert(pd->neuron_functs[i] == neuron_pool[i].get_type());
			assert(neuron_pool[i].get_id() == i);
		}
		for (uint16_t i = 0; i < f_neuron_pool_size; i += 1){
			assert(pd->f_neurons[i] == f_neuron_pool[i].get_int());
			assert(f_neuron_pool[i].get_id() == i);
		}	
	}	
	
		
	return true; //?
}

// Print DNA data to console, used for training
void ManagementData::dump_data() {
	//dumping new numbers
	printf ("     actual military_numbers (%lu):\n      {", pd->magic_numbers.size());
	uint16_t itemcounter = 1;
	uint16_t line_counter = 1;
	for (const auto& item : pd->magic_numbers) {
		printf (" %3d %s",item,(&item != &pd->magic_numbers.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");
	
	printf ("     actual neuron setup:\n      ");
	printf ("{ ");
	itemcounter = 1;
	for (auto& item : neuron_pool) {
		printf (" %3d %s",item.get_weight(),(&item != &neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n      { ");
	itemcounter = 1;	
	for (auto& item : neuron_pool) {
		printf (" %3d %s",item.get_type(),(&item != &neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");


	printf ("     actual f-neuron setup:\n      ");
	printf ("{ ");
	itemcounter = 1;
	for (auto& item : f_neuron_pool) {
		printf (" %8u %s",item.get_int(),(&item != &f_neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");
}

// Two functions to manipulate military number
int16_t ManagementData::get_military_number_at(uint8_t pos) {
	assert (pos < magic_numbers_size);
	return pd->magic_numbers[pos];
}

void ManagementData::set_military_number_at(const uint8_t pos, int16_t value) {
	assert (pos < magic_numbers_size);
	
	while (pos >= pd->magic_numbers.size()) {
		pd->magic_numbers.push_back(0);
	}
	
	if (value < -100) {
			value = -100;
	}
	if (value > 100) {
			value = 100;
	}
	pd->magic_numbers[pos] = value;
}

uint16_t MineTypesObserver::total_count() const {
	return in_construction + finished;
}

// this is used to count militarysites by their size
MilitarySiteSizeObserver::MilitarySiteSizeObserver() : in_construction(0), finished(0) {
}

// this represents a scheduler task
SchedulerTask::SchedulerTask(const uint32_t time,
                             const Widelands::SchedulerTaskId t,
                             const uint8_t p,
                             const char* d)
   : due_time(time), id(t), priority(p), descr(d) {
}

bool SchedulerTask::operator<(SchedulerTask other) const {
	return priority > other.priority;
}

// List of blocked fields with block time, with some accompanying functions
void BlockedFields::add(Widelands::Coords coords, uint32_t till) {
	const uint32_t hash = coords.hash();
	if (blocked_fields_.count(hash) == 0) {
		blocked_fields_.insert(std::pair<uint32_t, uint32_t>(hash, till));
	} else if (blocked_fields_[hash] < till) {
		blocked_fields_[hash] = till;
	}
	// The third possibility is that a field has been already blocked for longer time than 'till'
}

uint32_t BlockedFields::count() {
	return blocked_fields_.size();
}

void BlockedFields::remove_expired(uint32_t gametime) {
	std::vector<uint32_t> fields_to_remove;
	for (auto field : blocked_fields_) {
		if (field.second < gametime) {
			fields_to_remove.push_back(field.first);
		}
	}
	while (!fields_to_remove.empty()) {
		blocked_fields_.erase(fields_to_remove.back());
		fields_to_remove.pop_back();
	}
}

bool BlockedFields::is_blocked(Coords coords) {
	return (blocked_fields_.count(coords.hash()) != 0);
}

FlagsForRoads::Candidate::Candidate(uint32_t coords, int32_t distance, bool economy)
   : coords_hash(coords), air_distance(distance), different_economy(economy) {
	new_road_possible = false;
	accessed_via_roads = false;
	// Values are only very rough, and are dependant on the map size
	new_road_length = 2 * Widelands::kMapDimensions.at(Widelands::kMapDimensions.size() - 1);
	current_roads_distance = 2 * (Widelands::kMapDimensions.size() - 1);  // must be big enough
	reduction_score = -air_distance;  // allows reasonable ordering from the start
}

bool FlagsForRoads::Candidate::operator<(const Candidate& other) const {
	if (reduction_score == other.reduction_score) {
		return coords_hash < other.coords_hash;
	} else {
		return reduction_score > other.reduction_score;
	}
}

bool FlagsForRoads::Candidate::operator==(const Candidate& other) const {
	return coords_hash == other.coords_hash;
}

void FlagsForRoads::Candidate::calculate_score() {
	if (!new_road_possible) {
		reduction_score = kRoadNotFound - air_distance;  // to have at least some ordering preserved
	} else if (different_economy) {
		reduction_score = kRoadToDifferentEconomy - air_distance - 2 * new_road_length;
	} else if (!accessed_via_roads) {
		if (air_distance + 6 > new_road_length) {
			reduction_score = kShortcutWithinSameEconomy - air_distance - 2 * new_road_length;
		} else {
			reduction_score = kRoadNotFound;
		}
	} else {
		reduction_score = current_roads_distance - 2 * new_road_length;
	}
}

void FlagsForRoads::print() {  // this is for debugging and development purposes
	for (auto& candidate_flag : queue) {
		log("   %starget: %3dx%3d, saving: %5d (%3d), air distance: %3d, new road: %6d, score: %5d "
		    "%s\n",
		    (candidate_flag.reduction_score >= min_reduction && candidate_flag.new_road_possible) ?
		       "+" :
		       " ",
		    Coords::unhash(candidate_flag.coords_hash).x,
		    Coords::unhash(candidate_flag.coords_hash).y,
		    candidate_flag.current_roads_distance - candidate_flag.new_road_length, min_reduction,
		    candidate_flag.air_distance, candidate_flag.new_road_length,
		    candidate_flag.reduction_score,
		    (candidate_flag.new_road_possible) ? ", new road possible" : " ");
	}
}

// Queue is ordered but some target flags are only estimations so we take such a candidate_flag
// first
bool FlagsForRoads::get_best_uncalculated(uint32_t* winner) {
	for (auto& candidate_flag : queue) {
		if (!candidate_flag.new_road_possible) {
			*winner = candidate_flag.coords_hash;
			return true;
		}
	}
	return false;
}

// Road from starting flag to this flag can be built
void FlagsForRoads::road_possible(Widelands::Coords coords, uint32_t distance) {
	// std::set does not allow updating
	Candidate new_candidate_flag = Candidate(0, 0, false);
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == coords.hash()) {
			new_candidate_flag = candidate_flag;
			assert(new_candidate_flag.coords_hash == candidate_flag.coords_hash);
			queue.erase(candidate_flag);
			break;
		}
	}

	new_candidate_flag.new_road_length = distance;
	new_candidate_flag.new_road_possible = true;
	new_candidate_flag.calculate_score();
	queue.insert(new_candidate_flag);
}

// Remove the flag from candidates as interconnecting road is not possible
void FlagsForRoads::road_impossible(Widelands::Coords coords) {
	const uint32_t hash = coords.hash();
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == hash) {
			queue.erase(candidate_flag);
			return;
		}
	}
}

// Updating walking distance over existing roads
// Queue does not allow modifying its members so we erase and then eventually insert modified member
void FlagsForRoads::set_road_distance(Widelands::Coords coords, int32_t distance) {
	const uint32_t hash = coords.hash();
	Candidate new_candidate_flag = Candidate(0, 0, false);
	bool replacing = false;
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == hash) {
			assert(!candidate_flag.different_economy);
			if (distance < candidate_flag.current_roads_distance) {
				new_candidate_flag = candidate_flag;
				queue.erase(candidate_flag);
				replacing = true;
				break;
			}
			break;
		}
	}
	if (replacing) {
		new_candidate_flag.current_roads_distance = distance;
		new_candidate_flag.accessed_via_roads = true;
		new_candidate_flag.calculate_score();
		queue.insert(new_candidate_flag);
	}
}

bool FlagsForRoads::get_winner(uint32_t* winner_hash) {
	// If AI can ask for 2nd position, but there is only one viable candidate
	// we return the first one of course
	bool has_winner = false;
	for (auto candidate_flag : queue) {
		if (candidate_flag.reduction_score < min_reduction || !candidate_flag.new_road_possible) {
			continue;
		}
		assert(candidate_flag.air_distance > 0);
		assert(candidate_flag.reduction_score >= min_reduction);
		assert(candidate_flag.new_road_possible);
		*winner_hash = candidate_flag.coords_hash;
		has_winner = true;

		if (std::rand() % 3 > 0) {
			// with probability of 2/3 we accept this flag
			return true;
		}
	}

	if (has_winner) {
		return true;
	}
	return false;
}

// This is an struct that stores strength of players, info on teams and provides some outputs from
// these data
PlayersStrengths::PlayerStat::PlayerStat() : team_number(0), is_enemy(false), players_power(0), old_players_power(0), old60_players_power(0),players_casualities(0) {
}
PlayersStrengths::PlayerStat::PlayerStat(Widelands::TeamNumber tc, bool e, uint32_t pp, uint32_t op, uint32_t o60p, uint32_t cs, uint32_t land, uint32_t oland, uint32_t o60l)
   : team_number(tc), is_enemy(e), players_power(pp),  old_players_power(op), old60_players_power(o60p), players_casualities(cs), players_land(land),
	old_players_land(oland), old60_players_land(o60l)  {
	 last_time_seen = kNever;  
}

// Inserting/updating data
void PlayersStrengths::add(Widelands::PlayerNumber pn, Widelands::PlayerNumber opn, Widelands::TeamNumber mytn,
   Widelands::TeamNumber pltn, uint32_t pp, uint32_t op, uint32_t o60p, uint32_t cs, uint32_t land, uint32_t oland,  uint32_t o60l) {
	if (all_stats.count(opn) == 0) {
		bool enemy = false;
		if ( pn == opn ) {
			;
		} else if (pltn == 0 || mytn == 0) {
			enemy = true;
		} else if (pltn != mytn) {
			enemy = true;			
		}
		this_player_number = pn;
		//printf (" %d PlayersStrengths: player %d / team: %d - is%s enemy\n", pn, opn, pltn, (enemy)?"":" not");
		all_stats.insert(std::pair<Widelands::PlayerNumber, PlayerStat>(opn, PlayerStat(pltn, enemy, pp, op, o60p, cs, land, oland, o60l)));
	} else {
		all_stats[opn].players_power = pp;
		all_stats[opn].old_players_power = op;
		all_stats[opn].old60_players_power = o60p;
		all_stats[opn].players_casualities = cs;
		all_stats[opn].players_land = land;
		all_stats[opn].old_players_land = oland;
		all_stats[opn].old60_players_land = oland;
	}
}

void PlayersStrengths::recalculate_team_power() {
	team_powers.clear();
	for (auto& item : all_stats) {
		if (item.second.team_number > 0) {  // is a member of a team
			if (team_powers.count(item.second.team_number) > 0) {
				team_powers[item.second.team_number] += item.second.players_power;
			} else {
				team_powers[item.second.team_number] = item.second.players_power;
			}
		}
	}
}

bool PlayersStrengths::any_enemy_seen_lately(const uint32_t gametime){
	for (auto& item : all_stats) {
		if (item.second.is_enemy && player_seen_lately(item.first, gametime)) {
			return true;
		}  // is a member of a team
	}
	return false;
}

uint8_t PlayersStrengths::enemies_seen_lately_count(const uint32_t gametime){
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (item.second.is_enemy && player_seen_lately(item.first, gametime)) {
			count +=1;
		}  // is a member of a team
	}
	return count;
}

uint32_t PlayersStrengths::enemy_last_seen(){
	uint32_t time = 0;
	for (auto& item : all_stats) {
		if (item.second.last_time_seen == kNever){
			continue;
		}
		if (item.second.is_enemy && item.second.last_time_seen > time) {
			time = item.second.last_time_seen;
		}  
	}
	if (time == 0) {return kNever;}
	return time;
}



void PlayersStrengths::set_last_time_seen(const uint32_t seentime, Widelands::PlayerNumber pn){
	assert(all_stats.count(pn) > 0);
	all_stats[pn].last_time_seen = seentime;
}

bool PlayersStrengths::get_is_enemy(Widelands::PlayerNumber pn){
	assert(all_stats.count(pn) > 0);
	return all_stats[pn].is_enemy;

}

bool PlayersStrengths::player_seen_lately( Widelands::PlayerNumber pn, const uint32_t gametime){
	assert(all_stats.count(pn) > 0);
	if (all_stats[pn].last_time_seen == kNever){
		return false;
	}
	if (all_stats[pn].last_time_seen + static_cast<uint32_t>(2 * 60 * 1000) > gametime){
		return true;
	}
	return false;
}

// This is strength of player
uint32_t PlayersStrengths::get_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_power;
	};
	return 0;
}

// This is land size owned by player
uint32_t PlayersStrengths::get_player_land(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_land;
	};
	return 0;
}

uint32_t PlayersStrengths::get_visible_enemies_power(const uint32_t gametime){
	uint32_t pw = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first) && player_seen_lately(item.first, gametime)) {
			pw += item.second.players_power;
		}
	}
	return pw;
}

uint32_t PlayersStrengths::get_enemies_average_power(){
	uint32_t sum = 0;
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			sum += item.second.players_power;
			count += 1;
		}
	}
	if (count > 0) {
		return sum/count;
	}
	return 0;
}

uint32_t PlayersStrengths::get_enemies_average_land(){
	uint32_t sum = 0;
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			sum += item.second.players_land;
			count += 1;
		}
	}
	if (count > 0) {
		return sum/count;
	}
	return 0;
}

uint32_t PlayersStrengths::get_enemies_max_power(){
	uint32_t power = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			power=std::max<uint32_t>(power, item.second.players_power);
		}
	}
	return power;
}

uint32_t PlayersStrengths::get_enemies_max_land(){
	uint32_t land = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			land=std::max<uint32_t>(land, item.second.players_land);
		}
	}
	return land;
}

uint32_t PlayersStrengths::get_old_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].old_players_power;
	};
	return 0;
}

uint32_t PlayersStrengths::get_old60_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].old60_players_power;
	};
	return 0;
}

uint32_t PlayersStrengths::get_old_player_land(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) == 0) {
		printf (" %d: Players statistics are still empty\n", pn);
		return 0;
	}
	return all_stats[pn].old_players_land;
}

uint32_t PlayersStrengths::get_old60_player_land(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) == 0) {
		printf (" %d: Players statistics are still empty\n", pn);
		return 0;
	}
	return all_stats[pn].old60_players_land;
}


uint32_t PlayersStrengths::get_old_visible_enemies_power(const uint32_t gametime){
	uint32_t pw = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first) && player_seen_lately(item.first, gametime)) {
			pw += item.second.old_players_power;
		}
	}
	return pw;
}

// This is casualities of player
uint32_t PlayersStrengths::get_player_casualities(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_casualities;
	};
	return 0;
}

// This is strength of player plus third of strength of other members of his team
uint32_t PlayersStrengths::get_modified_player_power(Widelands::PlayerNumber pn) {
	uint32_t result = 0;
	Widelands::TeamNumber team = 0;
	if (all_stats.count(pn) > 0) {
		result = all_stats[pn].players_power;
		team = all_stats[pn].team_number;
	};
	if (team > 0 && team_powers.count(team) > 0) {
		result = result + (team_powers[team] - result) / 3;
	};
	return result;
}

bool PlayersStrengths::players_in_same_team(Widelands::PlayerNumber pl1,
                                            Widelands::PlayerNumber pl2) {
	assert(all_stats.count(pl1) > 0);
	assert(all_stats.count(pl2) > 0);
	if (pl1 == pl2) {
		return false;
	} else if (all_stats[pl1].team_number > 0 &&
		       all_stats[pl1].team_number == all_stats[pl2].team_number) {
		// team number 0 = no team
		return true;
	} else {
		return false;
	}
}

bool PlayersStrengths::strong_enough(Widelands::PlayerNumber pl) {
	if (all_stats.count(pl) == 0) {
		return false;
	}
	uint32_t my_strength = all_stats[pl].players_power;
	uint32_t strongest_opponent_strength = 0;
	for (auto item : all_stats) {
		if (!players_in_same_team(item.first, pl) && pl != item.first) {
			if (get_modified_player_power(item.first) > strongest_opponent_strength) {
				strongest_opponent_strength = get_modified_player_power(item.first);
			}
		}
	}
	return my_strength > strongest_opponent_strength + 50;
}

}  // namespace WIdelands
