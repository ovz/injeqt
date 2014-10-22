/*
 * %injeqt copyright begin%
 * Copyright 2014 Rafał Malinowski (rafal.przemyslaw.malinowski@gmail.com)
 * %injeqt copyright end%
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "injector-core.h"

#include <injeqt/exception/ambiguous-types.h>
#include <injeqt/exception/unavailable-required-types.h>
#include <injeqt/exception/unknown-type.h>
#include <injeqt/module.h>

#include "interfaces-utils.h"
#include "provider-by-default-constructor.h"
#include "provider-ready.h"
#include "provider.h"
#include "module-impl.h"
#include "required-to-instantiate.h"
#include "resolve-dependencies.h"
#include "resolved-dependency.h"

#include <cassert>

namespace injeqt { namespace internal {

injector_core::injector_core()
{
}

injector_core::injector_core(std::vector<std::unique_ptr<provider>> &&all_providers)
{
	auto all_providers_size = all_providers.size();
	_available_providers = providers{std::move(all_providers)};

	// some types were removed, because of duplication
	if (_available_providers.size() != all_providers_size)
		throw exception::ambiguous_types{}; // TODO: find a way to extract type names

	_types_model = create_types_model();

	auto required_types = std::vector<type>{};
	for (auto &&p : _available_providers)
		for (auto &&r : p->required_types())
			required_types.push_back(r);

	auto unavailable_required_types = match(types{required_types}, _types_model.available_types()).unmatched_1;
	if (!unavailable_required_types.empty())
	{
		auto message = std::string{};
		for (auto &&t : unavailable_required_types)
		{
			message.append(t.name());
			message.append("\n");
		}
		throw exception::unavailable_required_types{message};
	}
}

types_model injector_core::create_types_model() const
{
	auto result = std::vector<type>{};
	std::transform(std::begin(_available_providers), std::end(_available_providers), std::back_inserter(result),
		[](const std::unique_ptr<provider> &c){ return c->provided_type(); });
	return make_types_model(result);
}

QObject * injector_core::get(const type &interface_type)
{
	assert(!interface_type.is_empty());
	assert(!interface_type.is_qobject());

	auto implementation_type_it = _types_model.available_types().get(interface_type);
	if (implementation_type_it == end(_types_model.available_types()))
		throw exception::unknown_type{interface_type.name()};

	auto implementation_type = implementation_type_it->implementation_type();
	auto object_it = _objects.get(implementation_type);
	if (object_it != end(_objects))
		return object_it->object();

	_objects = objects_with(_objects, implementation_type);
	return _objects.get(implementation_type)->object();
}

implementations injector_core::objects_with(implementations objects, const type &implementation_type)
{
	auto types_to_instantiate = required_to_instantiate(implementation_type, _types_model, objects);
	return objects_with(objects, types_to_instantiate);
}

implementations injector_core::objects_with(implementations objects, const types &types_to_instantiate)
{
	auto objects_to_resolve = std::vector<implementation>{};
	auto objects_to_store = std::vector<implementation>{};
	for (auto &&type_to_instantiate : types_to_instantiate)
	{
		auto provider_it = _available_providers.get(type_to_instantiate);
		assert(provider_it != end(_available_providers));

		for (auto &&required_type : provider_it->get()->required_types())
			objects = objects_with(objects, required_type);
	}

	for (auto &&type_to_instantiate : types_to_instantiate)
	{
		if (objects.get(type_to_instantiate) != end(objects))
			continue;

		auto provider_it = _available_providers.get(type_to_instantiate);
		auto instance = provider_it->get()->provide(*this);

		auto i = make_implementation(type_to_instantiate, instance);
		objects_to_resolve.emplace_back(i);

		auto interfaces = extract_interfaces(type_to_instantiate);
		auto matched = match(interfaces, _types_model.available_types()).matched;
		for (auto &&m : matched)
		{
			auto i = implementation{m.first, instance}; // no need to check preconditions again with make_implementation
			objects_to_store.emplace_back(i);
		}
	}

	objects.merge(implementations{objects_to_store});

	for (auto &&object_to_resolve : objects_to_resolve)
	{
		auto to_resolve = _types_model.mapped_dependencies().get(object_to_resolve.interface_type())->dependency_list();
		auto resolved_dependencies = resolve_dependencies(to_resolve, objects);
		assert(resolved_dependencies.unresolved.empty());

		for (auto &&resolved : resolved_dependencies.resolved)
		{
			assert(object_to_resolve.interface_type() == resolved.setter().object_type());
			resolved.apply_on(object_to_resolve.object());
		}
	}

	return objects;
}

}}