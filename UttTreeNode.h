#ifndef UTTTREENODE_H
#define UTTTREENODE_H

class UttTreeNode
{
public:
  UttTreeNode(double ultDist, double weight, int subRef = -1, UttTreeNode *ls = nullptr, UttTreeNode *rs = nullptr, bool visible = true);
  ~UttTreeNode();
  double getUltDist() const;
  double getWeight() const;
  int getSubRef() const;
  bool getVisible() const;
  UttTreeNode * getLeftSon() const;
  UttTreeNode * getRightSon() const;
  void setVisible(bool visible);

private:
  double m_ultDist;
  int m_subRef;
  double m_weight;
  UttTreeNode *m_ls;
  UttTreeNode *m_rs;
  bool m_visible;
};

#endif
