#pragma once

#include "IdxPool.h"
#include <hash_map>
#include "tinyxml\tinyxml.h"
namespace ldp
{
	class AbstractPanelObject
	{
	public:
		enum Type
		{
			TypeKeyPoint = 0x01,
			TypeLine = 0x02,
			TypeQuadratic = 0x04,
			TypeCubic = 0x08,
			TypeGroup = 0x10,
			TypePanelPolygon = 0x20,
			TypeSewing = 0x40,
			TypeAll = 0xffffffff,

			Type_End = 0,
		};
		enum SelectOp
		{
			SelectThis,
			SelectUnion,
			SelectUnionInverse,
			SelectAll,
			SelectNone,
			SelectInverse,
			SelectEnd
		};
	public:
		AbstractPanelObject()
		{
			m_id = IdxPool::requireIdx(0);
			s_globalIdxMap.insert(std::make_pair(m_id, this));
		}
		AbstractPanelObject(size_t id)
		{
			requireIdx(id);
		}
		AbstractPanelObject(const AbstractPanelObject& rhs)
		{
			requireIdx(rhs.m_id);
			m_highlighted = rhs.m_highlighted;
			m_selected = rhs.m_selected;
		}
		virtual ~AbstractPanelObject()
		{
			IdxPool::freeIdx(m_id);
			if (!isIdxMapUpdateDisabled())
			{
				auto iter = s_globalIdxMap.find(m_id);
				if (iter != s_globalIdxMap.end())
				{
					if (iter->second == this)
						s_globalIdxMap.erase(iter);
				}
			}
		}
		size_t getId()const { return m_id; }
		void setSelected(bool s) { m_selected = s; }
		bool isSelected()const { return m_selected; }
		void setHighlighted(bool s) { m_highlighted = s; }
		bool isHighlighted()const { return m_highlighted; }
		virtual Type getType()const = 0;
		std::string getTypeString()const { return s_typeStringMap[getType()]; }
		virtual void collectObject(std::vector<AbstractPanelObject*>& objs) = 0;
		virtual void collectObject(std::vector<const AbstractPanelObject*>& objs)const = 0;
		virtual AbstractPanelObject* clone()const = 0;
		static AbstractPanelObject* getPtrById(size_t id)
		{
			auto iter = s_globalIdxMap.find(id);
			if (iter != s_globalIdxMap.end())
				return iter->second;
			return nullptr;
		}

		virtual TiXmlElement* toXML(TiXmlNode* parent)const
		{
			TiXmlElement* ele = new TiXmlElement(getTypeString().c_str());
			parent->LinkEndChild(ele);
			ele->SetAttribute("id", m_id);
			return ele;
		}

		virtual void fromXML(TiXmlElement* self)
		{
			int id = 0;
			if (!self->Attribute("id", &id))
				throw std::exception(("cannot find id for " + getTypeString()).c_str());
			requireIdx(id);
		}
	protected:
		virtual AbstractPanelObject& operator = (const AbstractPanelObject& rhs)
		{
			requireIdx(rhs.getId());
			m_selected = rhs.m_selected;
			m_highlighted = rhs.m_highlighted;
			return *this;
		}
	protected:
		bool m_selected = false;
		bool m_highlighted = false;
		static std::hash_map<Type, std::string> s_typeStringMap;
	private:
		size_t m_id = 0;
	private:
		static std::hash_map<size_t, AbstractPanelObject*> s_globalIdxMap;
		static bool s_disableIdxMapUpdate;
		static std::hash_map<Type, std::string> generateTypeStringMap();
		void requireIdx(size_t id)
		{
			m_id = IdxPool::requireIdx(id);
			if (m_id && !isIdxMapUpdateDisabled())
			{
				auto iter = s_globalIdxMap.find(m_id);
				if (iter == s_globalIdxMap.end())
					s_globalIdxMap.insert(std::make_pair(m_id, this));
				else
					iter->second = this;
			}
		}
	public:
		static void disableIdxMapUpdate() { s_disableIdxMapUpdate = true; }
		static void enableIdxMapUpdate() { s_disableIdxMapUpdate = false; }
		static bool isIdxMapUpdateDisabled() { return s_disableIdxMapUpdate; }
	};
}