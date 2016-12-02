#pragma once
#include "ldpMat\ldp_basic_mat.h"
#include <set>
#include <hash_map>
#include "AbstractPanelObject.h"
namespace ldp
{
#pragma region --units
	class KeyPoint : public AbstractPanelObject
	{
	public:
		KeyPoint() : AbstractPanelObject() {}
		KeyPoint(size_t id) : AbstractPanelObject(id) {}
		KeyPoint(const Float2& p) : AbstractPanelObject(), position(p) {}
		KeyPoint(size_t id, const Float2& p) : AbstractPanelObject(id), position(p) {}
		KeyPoint(const KeyPoint& rhs) : AbstractPanelObject(rhs) { position = rhs.position; }
		virtual AbstractPanelObject* clone()const
		{
			return (AbstractPanelObject*)new KeyPoint(*this);
		}
		virtual Type getType()const { return TypeKeyPoint; }
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs)
		{
			objs.push_back(this);
		}
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const
		{
			objs.push_back(this);
		}
		virtual TiXmlElement* toXML(TiXmlNode* parent)const
		{
			TiXmlElement* ele = AbstractPanelObject::toXML(parent);
			ele->SetDoubleAttribute("x", position[0]);
			ele->SetDoubleAttribute("y", position[1]);
			return ele;
		}
		virtual void fromXML(TiXmlElement* self)
		{
			AbstractPanelObject::fromXML(self);
			double x = 0, y = 0;
			if (!self->Attribute("x", &x))
				throw std::exception(("cannot find x for " + getTypeString()).c_str());
			if (!self->Attribute("y", &y))
				throw std::exception(("cannot find y for " + getTypeString()).c_str());
			position[0] = x;
			position[1] = y;
		}
	public:
		Float2 position;
	};

	class AbstractShape : public AbstractPanelObject
	{
	public:
		AbstractShape() : AbstractPanelObject() {}
		AbstractShape(size_t id) : AbstractPanelObject(id) {}
		static AbstractShape* create(Type type, size_t id);
		static void create(std::vector<std::shared_ptr<AbstractShape>>& curves,
			const std::vector<Float2>& keyPoints, float fittingThre);
		static AbstractShape* create(std::string typeString, size_t id);
		AbstractShape(const std::vector<KeyPoint>& keyPoints) : AbstractShape()
		{
			m_keyPoints.clear();
			for (const auto& p : keyPoints)
				m_keyPoints.push_back(std::shared_ptr<KeyPoint>(new KeyPoint(p)));
			m_invalid = true;
		}
		virtual AbstractShape* clone()const;
		virtual Type getType()const = 0;
		virtual Float2 getPointByParam(float t)const = 0; // t \in [0, 1]
		const KeyPoint& getStartPoint()const
		{
			return *m_keyPoints[0];
		}
		const KeyPoint& getEndPoint()const
		{
			return *m_keyPoints.back();
		}
		int numKeyPoints()const
		{
			return (int)m_keyPoints.size();
		}
		void setKeyPoint(int i, const KeyPoint& p)
		{
			m_invalid = true;
			*m_keyPoints[i] = p;
		}
		const KeyPoint& getKeyPoint(int i)const
		{
			return *m_keyPoints[i];
		}
		virtual const std::vector<Float2>& samplePointsOnShape(float step)const;
		void translate(Float2 t)
		{
			for (auto& p : m_keyPoints)
				p->position += t;
			m_invalid = true;
		}
		void rotate(const ldp::Mat2f& R)
		{
			for (auto& p : m_keyPoints)
				p->position = R * p->position;
			m_invalid = true;
		}
		void rotateBy(const ldp::Mat2f& R, Float2 c)
		{
			for (auto& p : m_keyPoints)
				p->position = R * (p->position - c) + c;
			m_invalid = true;
		}
		void scale(Float2 s)
		{
			for (auto& p : m_keyPoints)
				p->position *= s;
			m_invalid = true;
		}
		void scaleBy(Float2 s, Float2 c)
		{
			for (auto& p : m_keyPoints)
				p->position = s*(p->position - c) + c;
			m_invalid = true;
		}
		void transform(const ldp::Mat3f& M)
		{
			for (auto& p : m_keyPoints)
			{
				ldp::Float3 p3(p->position[0], p->position[1], 1);
				p3 = M * p3;
				p->position[0] = p3[0] / p3[2];
				p->position[1] = p3[1] / p3[2];
			}
			m_invalid = true;
		}
		void unionBound(Float2& bmin, Float2& bmax)
		{
			for (const auto& p : m_keyPoints)
			{
				for (int k = 0; k < p->position.size(); k++)
				{
					bmin[k] = std::min(bmin[k], p->position[k]);
					bmax[k] = std::max(bmax[k], p->position[k]);
				}
			}
		}
		AbstractShape& reverse()
		{
			std::reverse(m_keyPoints.begin(), m_keyPoints.end());
			m_invalid = true;
			return *this;
		}
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs)
		{
			objs.push_back(this);
			for (auto p : m_keyPoints)
				p->collectObject(objs);
		}
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const
		{
			objs.push_back(this);
			for (auto p : m_keyPoints)
				p->collectObject(objs);
		}
		float getLength()const 
		{ 
			if (m_lengthInvalid)
				m_length = calcLength();
			m_lengthInvalid = false;
			return m_length; 
		}
		virtual TiXmlElement* toXML(TiXmlNode* parent)const
		{
			TiXmlElement* ele = AbstractPanelObject::toXML(parent);
			for (const auto& kp : m_keyPoints)
				kp->toXML(ele);
			return ele;
		}
		virtual void fromXML(TiXmlElement* self)
		{
			AbstractPanelObject::fromXML(self);
			m_keyPoints.clear();
			KeyPoint tmpKp;
			for (auto child = self->FirstChildElement(); child; child = child->NextSiblingElement())
			{
				if (child->Value() == tmpKp.getTypeString())
				{
					m_keyPoints.push_back(std::shared_ptr<KeyPoint>(new KeyPoint));
					m_keyPoints.back()->fromXML(child);
				}
			}
		}
	protected:
		virtual float calcLength()const;
		virtual AbstractShape& operator = (const AbstractShape& rhs)
		{
			AbstractPanelObject::operator=(rhs);
			m_keyPoints = rhs.m_keyPoints;
			m_samplePoints = rhs.m_samplePoints;
			m_invalid = rhs.m_invalid;
			m_lastSampleStep = rhs.m_lastSampleStep;
			m_length = rhs.m_length;
			return *this;
		}
	protected:
		std::vector<std::shared_ptr<KeyPoint>> m_keyPoints;
		mutable std::vector<Float2> m_samplePoints;
		mutable bool m_invalid = true;
		mutable bool m_lengthInvalid = true;
		mutable float m_lastSampleStep = 0;
		mutable float m_length = 0;
	};

	class Line : public AbstractShape
	{
	public:
		Line() : AbstractShape()
		{
			m_keyPoints.resize(2);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Line(size_t id) : AbstractShape(id)
		{
			m_keyPoints.resize(2);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Line(const std::vector<KeyPoint>& keyPoints) : AbstractShape(keyPoints)
		{
			assert(keyPoints.size() == 2);
		}
		virtual Type getType()const { return TypeLine; }
		virtual Float2 getPointByParam(float t)const
		{
			return m_keyPoints[0]->position * (1 - t) + m_keyPoints[1]->position * t;
		}
	protected:
		virtual float calcLength()const { return (m_keyPoints[1]->position - m_keyPoints[0]->position).length(); }
	};

	class Quadratic : public AbstractShape
	{
	public:
		Quadratic() : AbstractShape()
		{
			m_keyPoints.resize(3);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Quadratic(size_t id) : AbstractShape(id)
		{
			m_keyPoints.resize(3);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Quadratic(const std::vector<KeyPoint>& keyPoints) : AbstractShape(keyPoints)
		{
			assert(keyPoints.size() == 3);
		}
		virtual Type getType()const { return TypeQuadratic; }
		virtual Float2 getPointByParam(float t)const
		{
			return (1 - t) * ((1 - t)*m_keyPoints[0]->position + t*m_keyPoints[1]->position)
				+ t * ((1 - t)*m_keyPoints[1]->position + t*m_keyPoints[2]->position);
		}
	};

	class Cubic : public AbstractShape
	{
	public:
		Cubic() : AbstractShape()
		{
			m_keyPoints.resize(4);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Cubic(size_t id) : AbstractShape(id)
		{
			m_keyPoints.resize(4);
			for (size_t k = 0; k < m_keyPoints.size(); k++)
				m_keyPoints[k].reset(new KeyPoint());
		}
		Cubic(const std::vector<KeyPoint>& keyPoints) : AbstractShape(keyPoints)
		{
			assert(keyPoints.size() == 4);
		}
		virtual Type getType()const { return TypeCubic; }
		virtual Float2 getPointByParam(float t)const
		{
			Float2 p1 = (1 - t) * ((1 - t)*m_keyPoints[0]->position + t*m_keyPoints[1]->position)
				+ t * ((1 - t)*m_keyPoints[1]->position + t*m_keyPoints[2]->position);
			Float2 p2 = (1 - t) * ((1 - t)*m_keyPoints[1]->position + t*m_keyPoints[2]->position)
				+ t * ((1 - t)*m_keyPoints[2]->position + t*m_keyPoints[3]->position);
			return (1 - t) * p1 + t * p2;
		}
	};

	typedef std::shared_ptr<KeyPoint> KeyPointPtr;
	typedef std::shared_ptr<AbstractShape> ShapePtr;
	typedef std::shared_ptr<Line> LinePtr;
	typedef std::shared_ptr<Quadratic> QaudraticPtr;
	typedef std::shared_ptr<Cubic> CubicPtr;

	class ShapeGroup : public std::vector<ShapePtr>, public AbstractPanelObject
	{
	public:
		ShapeGroup() : AbstractPanelObject(), std::vector<ShapePtr>()
		{
			m_bbox[0] = FLT_MAX;
			m_bbox[1] = -FLT_MAX;
		}
		ShapeGroup(size_t id) : AbstractPanelObject(id), std::vector<ShapePtr>()
		{
			m_bbox[0] = FLT_MAX;
			m_bbox[1] = -FLT_MAX;
		}
		virtual Type getType()const { return TypeGroup; }
		virtual AbstractPanelObject* clone()const
		{
			ShapeGroup* g = new ShapeGroup(*this);
			for (size_t i = 0; i < size(); i++)
				(*g)[i].reset((AbstractShape*)(*this)[i]->clone());
			return g;
		}
		void updateBound(Float2& bmin, Float2& bmax)
		{
			m_bbox[0] = FLT_MAX;
			m_bbox[1] = -FLT_MAX;
			for (size_t i = 0; i < size(); i++)
				(*this)[i]->unionBound(m_bbox[0], m_bbox[1]);
			for (int k = 0; k < bmin.size(); k++)
			{
				bmin[k] = std::min(bmin[k], m_bbox[0][k]);
				bmax[k] = std::max(bmax[k], m_bbox[1][k]);
			}
		}
		const Float2* bound()const { return m_bbox; }
		ShapeGroup& reverse()
		{
			for (size_t i = 0; i < size(); i++)
				(*this)[i]->reverse();
			std::reverse(begin(), end());
			return *this;
		}
		void collectKeyPoints(std::vector<Float2>& pts, float distThre = 0)
		{
			for (size_t i = 0; i < size(); i++)
			{
				const auto& sp = (*this)[i];
				for (int j = 0; j < sp->numKeyPoints(); j++)
				{
					if (pts.size())
					{
						if ((pts.back() - sp->getKeyPoint(j).position).length() < distThre)
							continue;
					}
					pts.push_back(sp->getKeyPoint(j).position);
				} // end for j
			} // end for i
		}
		void collectSamplePoints(std::vector<Float2>& pts, float step, float distThre = 0)
		{
			for (size_t i = 0; i < size(); i++)
			{
				const auto& sp = (*this)[i];
				const auto& ps = sp->samplePointsOnShape(step);
				for (int j = 0; j < ps.size(); j++)
				{
					if (pts.size())
					{
						if ((pts.back() - ps[j]).length() < distThre)
							continue;
					}
					pts.push_back(ps[j]);
				} // end for j
			} // end for i
		}
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs)
		{
			objs.push_back(this);
			for (auto p : (*this))
				p->collectObject(objs);
		}
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const
		{
			objs.push_back(this);
			for (auto p : (*this))
				p->collectObject(objs);
		}
		
		virtual TiXmlElement* toXML(TiXmlNode* parent)const
		{
			TiXmlElement* ele = AbstractPanelObject::toXML(parent);
			for (const auto& shape : (*this))
				shape->toXML(ele);
			return ele;
		}
		virtual void fromXML(TiXmlElement* self)
		{
			AbstractPanelObject::fromXML(self);
			clear();
			for (auto child = self->FirstChildElement(); child; child = child->NextSiblingElement())
			{
				auto ptr = AbstractShape::create(child->Value(), 0);
				if (ptr)
					ptr->fromXML(child);
				push_back(std::shared_ptr<AbstractShape>(ptr));
			}
			updateBound(m_bbox[0], m_bbox[1]);
		}
	protected:
		Float2 m_bbox[2];
	};

	typedef std::shared_ptr<ShapeGroup> ShapeGroupPtr;
#pragma endregion

	class PanelPolygon : public AbstractPanelObject
	{
	public:
		typedef ShapeGroup Polygon;
		typedef ShapeGroup Dart;
		typedef ShapeGroup InnerLine;
		typedef std::shared_ptr<Polygon> PolygonPtr;
		typedef std::shared_ptr<Dart> DartPtr;
		typedef std::shared_ptr<InnerLine> InnerLinePtr;
	public:
		PanelPolygon();
		PanelPolygon(size_t id);
		~PanelPolygon();

		void clear();

		virtual Type getType()const { return TypePanelPolygon; }
		virtual AbstractPanelObject* clone()const;
		void create(PolygonPtr outerPoly);
		void addDart(DartPtr dart);
		void addInnerLine(InnerLinePtr line);
		const std::vector<DartPtr>& darts()const { return m_darts; }
		const std::vector<InnerLinePtr>& innerLines()const { return m_innerLines; }
		const PolygonPtr& outerPoly()const { return m_outerPoly; }
		std::vector<DartPtr>& darts() { return m_darts; }
		std::vector<InnerLinePtr>& innerLines() { return m_innerLines; }
		const PolygonPtr& outerPoly() { return m_outerPoly; }

		bool select(int idx, SelectOp op);
		bool select(const std::set<int>& indices, SelectOp op);
		void highLight(int idx, int lastIdx);

		void updateBound(Float2& bmin, Float2& bmax);
		const Float2* bound()const { return m_bbox; }
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs);
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const;		
		virtual TiXmlElement* toXML(TiXmlNode* parent)const;
		virtual void fromXML(TiXmlElement* self);
	private:
		PolygonPtr m_outerPoly;		
		std::vector<DartPtr> m_darts;
		std::vector<InnerLinePtr> m_innerLines;
		ldp::Float2 m_bbox[2];
		std::vector<AbstractPanelObject*> m_tmpbufferObj;
	};

	class Sewing : public AbstractPanelObject
	{
	public:
		struct Unit
		{
			size_t id;
			bool reverse;
			Unit() :id(0), reverse(false) {}
			Unit(size_t i, bool d) :id(i), reverse(d) {}
		};
	public:
		void clear();
		bool empty()const { return m_firsts.size() == 0 || m_seconds.size() == 0; }
		const std::vector<Unit>& firsts()const { return m_firsts; }
		const std::vector<Unit>& seconds()const { return m_seconds; }
		std::vector<Unit>& firsts() { return m_firsts; }
		std::vector<Unit>& seconds() { return m_seconds; }
		void addFirst(Unit unit);
		void addSecond(Unit unit);
		void addFirsts(const std::vector<Unit>& unit);
		void addSeconds(const std::vector<Unit>& unit);
		void remove(size_t id);
		void remove(const std::set<size_t>& s);
		virtual Type getType()const { return TypeSewing; }
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs) { objs.push_back(this); }
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const { objs.push_back(this); }
		virtual Sewing* clone() const;
		bool select(int idx, SelectOp op);
		bool select(const std::set<int>& indices, SelectOp op);
		void highLight(int idx, int lastIdx);
		virtual TiXmlElement* toXML(TiXmlNode* parent)const;
		virtual void fromXML(TiXmlElement* self);

		bool isSame_ignoreOrder(const Sewing& rhs)const;
	protected:
		std::vector<Unit> m_firsts;
		std::vector<Unit> m_seconds;
		std::vector<AbstractPanelObject*> m_tmpbufferObj;
	};

	typedef std::shared_ptr<Sewing> SewingPtr;
}