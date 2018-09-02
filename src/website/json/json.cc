/*
 * Copyright (C) 2018 by the Widelands Development Team
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

#include "website/json/json.h"

// ########################## JSON Element #############################

namespace JSON {
const std::string JSON::Element::tab_ = "   ";

std::string Element::as_string() const {
	return "{\n" + children_as_string() + "}";
}

std::string Element::children_as_string() const {
	std::string result = "";
	if (!children_.empty()) {
		for (size_t i = 0; i < children_.size() - 1; ++i) {
			result += children_.at(i)->as_string() + ",\n";
		}
		result += children_.at(children_.size() - 1)->as_string() + "\n";
	}
	return result;
}

std::string Element::key_to_string(const std::string& value) const {
	return "\"" + value + "\": ";
}

// ########################## JSON Object #############################

Object::Object(int level) : JSON::Element(level) {
}

void Object::add_value(const std::string& key, const std::string& value) {
	values_.push_back(std::make_pair(key, value));
}

std::string Object::as_string() const {
	std::string result = "";
	std::string tabs = "";
	for (int i = 0; i < level_; ++i) {
		tabs += tab_;
	}

	result += tabs + "{\n";

	if (!values_.empty()) {
		for (size_t i = 0; i < values_.size() - 1; ++i) {
			const auto& element = values_.at(i);
			result +=
			   tabs + tab_ + key_to_string(element.first) + value_to_string(element.second) + ",\n";
		}
		const auto& element = values_.at(values_.size() - 1);
		result += tabs + tab_ + key_to_string(element.first) + value_to_string(element.second) +
		          (children_.empty() ? "\n" : ",\n");
	}

	result += children_as_string();

	result += tabs + "}";
	return result;
}

std::string Object::value_to_string(const std::string& value) const {
	return "\"" + value + "\"";
}
}  // namespace JSON
