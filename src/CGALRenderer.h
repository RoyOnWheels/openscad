#ifndef CGALRENDERER_H_
#define CGALRENDERER_H_

#include <string>
#include <map>
#include <list>
#include "visitor.h"
#include "nodecache.h"
#include "cgal.h"

#ifdef ENABLE_CGAL
extern CGAL_Nef_polyhedron3 minkowski3(CGAL_Nef_polyhedron3 a, CGAL_Nef_polyhedron3 b);
extern CGAL_Nef_polyhedron2 minkowski2(CGAL_Nef_polyhedron2 a, CGAL_Nef_polyhedron2 b);
#endif

using std::string;
using std::map;
using std::list;
using std::pair;

class CGALRenderer : public Visitor
{
public:
	enum CsgOp {UNION, INTERSECTION, DIFFERENCE, MINKOWSKI};
	// FIXME: If a cache is not give, we need to fix this ourselves
	CGALRenderer(const NodeCache<string> &dumpcache) : root(NULL), dumpcache(dumpcache) {}
  virtual ~CGALRenderer() {}

  virtual Response visit(const State &state, const AbstractNode &node);
 	virtual Response visit(const State &state, const AbstractIntersectionNode &node);
 	virtual Response visit(const State &state, const CsgNode &node);
 	virtual Response visit(const State &state, const TransformNode &node);
	virtual Response visit(const State &state, const AbstractPolyNode &node);

	QHash<QString, CGAL_Nef_polyhedron> &getCache() { return this->cache; }

 	CGAL_Nef_polyhedron renderCGALMesh(const AbstractNode &node);
	CGAL_Nef_polyhedron renderCGALMesh(const PolySet &polyset);

	// FIXME: Questionable design...
	static CGALRenderer *renderer() { return global_renderer; }
	static void setRenderer(CGALRenderer *r) { global_renderer = r; }
private:
  void addToParent(const State &state, const AbstractNode &node);
  bool isCached(const AbstractNode &node) const;
	QString mk_cache_id(const AbstractNode &node) const;
	void process(CGAL_Nef_polyhedron &target, const CGAL_Nef_polyhedron &src, CGALRenderer::CsgOp op);
	void applyToChildren(const AbstractNode &node, CGALRenderer::CsgOp op);

  string currindent;
  const AbstractNode *root;
  typedef list<pair<const AbstractNode *, QString> > ChildList;
  map<int, ChildList> visitedchildren;

  // FIXME: enforce some maximum cache size (old version had 100K vertices as limit)
	QHash<QString, CGAL_Nef_polyhedron> cache;
	const NodeCache<string> &dumpcache;

	static CGALRenderer *global_renderer;
};

#endif