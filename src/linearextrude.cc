/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "linearextrudenode.h"

#include "module.h"
#include "context.h"
#include "printutils.h"
#include "builtin.h"
#include "PolySetEvaluator.h"
#include "openscad.h" // get_fragments_from_r()
#include "mathc99.h" 

#include <sstream>
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <QFileInfo>

class LinearExtrudeModule : public AbstractModule
{
public:
	LinearExtrudeModule() { }
	virtual AbstractNode *evaluate(const Context *ctx, const ModuleInstantiation *inst) const;
};

AbstractNode *LinearExtrudeModule::evaluate(const Context *ctx, const ModuleInstantiation *inst) const
{
	LinearExtrudeNode *node = new LinearExtrudeNode(inst);

	std::vector<std::string> argnames;
	argnames += "file", "layer", "height", "origin", "scale", "center", "twist", "slices";
	std::vector<Expression*> argexpr;

	Context c(ctx);
	c.args(argnames, argexpr, inst->argnames, inst->argvalues);

	node->fn = c.lookup_variable("$fn").num;
	node->fs = c.lookup_variable("$fs").num;
	node->fa = c.lookup_variable("$fa").num;

	Value file = c.lookup_variable("file");
	Value layer = c.lookup_variable("layer", true);
	Value height = c.lookup_variable("height", true);
	Value convexity = c.lookup_variable("convexity", true);
	Value origin = c.lookup_variable("origin", true);
	Value scale = c.lookup_variable("scale", true);
	Value center = c.lookup_variable("center", true);
	Value twist = c.lookup_variable("twist", true);
	Value slices = c.lookup_variable("slices", true);

	if (!file.text.empty()) {
		PRINTF("DEPRECATED: Support for reading files in linear_extrude will be removed in future releases. Use a child import() instead.");
		node->filename = c.getAbsolutePath(file.text);
	}

	node->layername = layer.text;
	node->height = height.num;
	node->convexity = (int)convexity.num;
	origin.getv2(node->origin_x, node->origin_y);
	node->scale = scale.num;

	if (center.type == Value::BOOL)
		node->center = center.b;

	if (node->height <= 0)
		node->height = 100;

	if (node->convexity <= 0)
		node->convexity = 1;

	if (node->scale <= 0)
		node->scale = 1;

	if (twist.type == Value::NUMBER) {
		node->twist = twist.num;
		if (slices.type == Value::NUMBER) {
			node->slices = (int)slices.num;
		} else {
			node->slices = (int)fmax(2, fabs(get_fragments_from_r(node->height,
					node->fn, node->fs, node->fa) * node->twist / 360));
		}
		node->has_twist = true;
	}

	if (node->filename.empty()) {
		std::vector<AbstractNode *> evaluatednodes = inst->evaluateChildren();
		node->children.insert(node->children.end(), evaluatednodes.begin(), evaluatednodes.end());
	}

	return node;
}

void register_builtin_dxf_linear_extrude()
{
	builtin_modules["dxf_linear_extrude"] = new LinearExtrudeModule();
	builtin_modules["linear_extrude"] = new LinearExtrudeModule();
}

class PolySet *LinearExtrudeNode::evaluate_polyset(PolySetEvaluator *evaluator) const
{
	if (!evaluator) {
		PRINTF("WARNING: No suitable PolySetEvaluator found for %s module!", this->name().c_str());
		return NULL;
	}

	print_messages_push();

	PolySet *ps = evaluator->evaluatePolySet(*this);

	print_messages_pop();

	return ps;
}

std::string LinearExtrudeNode::toString() const
{
	std::stringstream stream;

	stream << this->name() << "(";
	if (!this->filename.empty()) { // Ignore deprecated parameters if empty 
		stream <<
			"file = \"" << this->filename << "\", "
			"cache = \"" << 	QFileInfo(QString::fromStdString(this->filename)) << "\", "
			"layer = \"" << this->layername << "\", "
			"origin = [ " << this->origin_x << " " << this->origin_y << " ], "
			"scale = " << this->scale << ", ";
	}
	stream <<
		"height = " << std::dec << this->height << ", "
		"center = " << (this->center?"true":"false") << ", "
		"convexity = " << this->convexity;
	
	if (this->has_twist) {
		stream << ", twist = " << this->twist << ", slices = " << this->slices;
	}
	stream << ", $fn = " << this->fn << ", $fa = " << this->fa << ", $fs = " << this->fs << ")";
	
	return stream.str();
}