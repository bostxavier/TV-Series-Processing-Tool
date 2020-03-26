#include "DendogramNode.h"

DendogramNode::DendogramNode(double ultDist, double weight, int label, DendogramNode *ls, DendogramNode *rs, bool visible)
  : m_ultDist(ultDist),
    m_weight(weight),
    m_label(label),
    m_ls(ls),
    m_rs(rs),
    m_visible(visible)
{
}

DendogramNode::~DendogramNode()
{
}

///////////////
// accessors //
///////////////

double DendogramNode::getUltDist() const
{
  return m_ultDist;
}

double DendogramNode::getWeight() const
{
  return m_weight;
}

int DendogramNode::getLabel() const
{
  return m_label;
}

bool DendogramNode::getVisible() const
{
  return m_visible;
}

DendogramNode * DendogramNode::getLeftSon() const
{
  return m_ls;
}

DendogramNode * DendogramNode::getRightSon() const
{
  return m_rs;
}

///////////////
// modifiers //
///////////////

void DendogramNode::setVisible(bool visible)
{
  m_visible = visible;
}
