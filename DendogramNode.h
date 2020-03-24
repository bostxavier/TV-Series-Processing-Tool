#ifndef DENDOGRAMNODE_H
#define DENDOGRAMNODE_H

class DendogramNode
{
public:
  DendogramNode(double ultDist, double weight, int label, DendogramNode *ls = nullptr, DendogramNode *rs = nullptr, bool visible = true);
  ~DendogramNode();

  double getUltDist() const;
  double getWeight() const;
  int getLabel() const;
  bool getVisible() const;
  DendogramNode * getLeftSon() const;
  DendogramNode * getRightSon() const;

  void setVisible(bool visible);

private:
  double m_ultDist;
  double m_weight;
  int m_label;
  DendogramNode *m_ls;
  DendogramNode *m_rs;
  bool m_visible;
};

#endif
