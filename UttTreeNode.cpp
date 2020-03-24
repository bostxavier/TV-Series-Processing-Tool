#include "UttTreeNode.h"

UttTreeNode::UttTreeNode(double ultDist, double weight, int subRef, UttTreeNode *ls, UttTreeNode *rs, bool visible)
  : m_ultDist(ultDist),
    m_subRef(subRef),
    m_weight(weight),
    m_ls(ls),
    m_rs(rs),
    m_visible(visible)
{
}

UttTreeNode::~UttTreeNode()
{
}

///////////////
// accessors //
///////////////

double UttTreeNode::getUltDist() const
{
  return m_ultDist;
}

double UttTreeNode::getWeight() const
{
  return m_weight;
}

int UttTreeNode::getSubRef() const
{
  return m_subRef;
}

bool UttTreeNode::getVisible() const
{
  return m_visible;
}

UttTreeNode * UttTreeNode::getLeftSon() const
{
  return m_ls;
}

UttTreeNode * UttTreeNode::getRightSon() const
{
  return m_rs;
}

///////////////
// modifiers //
///////////////

void UttTreeNode::setVisible(bool visible)
{
  m_visible = visible;
}
