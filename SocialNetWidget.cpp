#include <QtMath>
#include <QImage>

#include <QDebug>

#include <GL/glu.h>

#include "SocialNetWidget.h"

SocialNetWidget::SocialNetWidget(QWidget *parent)
  : // QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
  QWidget(parent),
    m_twoDim(true),
    m_scale(1.0),
    m_edgeScale(0.5),
    m_filterWeight(0.0),
    m_minRadius(0.0024),
    m_sceneIndex(0),
    m_maxRadius(0.05),
    m_displayLabels(false)
{
  m_lastPos.setX(0);
  m_lastPos.setY(0);

  setMinimumSize(700, 700);
  setMouseTracking(true);

  m_colors << QColor(219, 213, 185) << QColor(224, 199, 30);
  m_colors << QColor(186, 22, 63) << QColor(255, 250, 129);
  m_colors << QColor(181, 225, 174) << QColor(154, 206, 223);
  m_colors << QColor(249, 149, 51) << QColor(252, 169, 133);
  m_colors << QColor(193, 179, 215) << QColor(116, 161, 97);
  m_colors << QColor(133, 202, 93) << QColor(72, 181, 163);
  m_colors << QColor(69, 114, 147) << QColor(249, 150, 182);
  m_colors << QColor(117, 137, 191) << QColor(84, 102, 84);

  // m_refColor = QColor::fromRgb(178, 34, 34, 255);
  m_refColor = m_colors[3];
  // m_refColor = QColor::fromRgb(100, 100, 100, 255);
}

/*
void SocialNetWidget::initializeGL()
{
  qreal bound = qSqrt(3.0) + m_maxRadius;

  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 50.0 };
  GLfloat light_position[] = { bound, bound, bound, 0.0 };
  glClearColor(1.0, 1.0, 1.0, 0.0);
  glShadeModel(GL_SMOOTH);

  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
}

void SocialNetWidget::resizeGL(int width, int height)
{
  adjustViewPort(width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  qreal bound = qSqrt(3.0) + m_maxRadius;
  glOrtho(-bound, bound, -bound, bound, -bound, bound);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  m_projMat.setToIdentity();
  m_projMat.ortho(-bound, bound, -bound, bound, -bound, bound);

  m_mvMat.setToIdentity();
}

void SocialNetWidget::paintGL()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // drawAxes();
  // drawEdges();
  drawVertices();

  if (m_displayLabels)
    displayLabels();

  glFlush();
}

void SocialNetWidget::drawAxes()
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHTING);
  
  qglColor(QColor(Qt::green));

  glBegin(GL_LINES);
  qreal bound = qSqrt(3.0);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(bound, 0.0, 0.0);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(0.0, bound, 0.0);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(0.0, 0.0, bound);
  glEnd();
  
  renderText(0.95, 0.05, 0.0, "x");
  renderText(0.05, 0.95, 0.0, "y");
  renderText(0.0, 0.05, 0.95, "z");

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
}

void SocialNetWidget::drawVertices()
{
  GLUquadricObj *qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  gluQuadricNormals(qobj, GLU_SMOOTH);

  // drawing vertices
  for (int i(0); i < m_selVertices.size(); i++) {

    // retrieve vertex index
    int index = m_selVertices[i];

    // retrieve vertex degree
    qreal weight = m_verticesViews[m_sceneIndex][index].getWeight();
    
    // set ambient and diffuse colors
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, m_verticesColor[m_sceneIndex][index]);

    // retrieve vertex coordinates
    QVector3D v = m_verticesViews[m_sceneIndex][index].getV();
    
    qreal x = v.x();
    qreal y = v.y();
    qreal z = v.z();

    // move to vertex
    glPushMatrix();
    glTranslatef(x, y, z);
    
    // draw sphere if active vertex
    qreal radius = m_minRadius + (m_maxRadius - m_minRadius) * weight;
    if (weight > 0.0 && (m_activeVertices.isEmpty() || m_activeVertices.contains(index))) {
      
      int slices = static_cast<int>(150 * qSqrt(radius));
      int stacks = slices;

      gluSphere(qobj, radius / m_scale, slices, stacks);
    }

    glPopMatrix();
  }
}

void SocialNetWidget::drawEdges()
{
  GLUquadricObj *qobj = gluNewQuadric();
  gluQuadricDrawStyle(qobj, GLU_FILL);
  gluQuadricNormals(qobj, GLU_SMOOTH);

  // drawing edges
  for (int i(0); i < m_selEdges.size(); i++) {

    // retrieve edge index
    int index = m_selEdges[i];

    // retrieve edge weight
    qreal weight = m_edgesViews[m_sceneIndex][index].getWeight();

    // retrieve vertices coordinates
    QVector3D v1 = m_edgesViews[m_sceneIndex][index].getV1();
    QPair<qreal, QPair<qreal, QVector3D> > v2 = m_edgesViews[m_sceneIndex][index].getV2();

    // retrieve ajacent vertices indices
    int v1Id = m_edgesViews[m_sceneIndex][index].getV1Index();
    int v2Id = m_edgesViews[m_sceneIndex][index].getV2Index();

    // set ambient and diffuse colors
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, m_edgesColor[m_sceneIndex][index]);

    // retrieve edge length
    qreal length = v2.first;

    // move to first vertex
    glPushMatrix();
    glTranslatef(v1.x(), v1.y(), v1.z());
    
    // rotation needed to move z to v2
    QPair<qreal, QVector3D> moveToV2 = v2.second;
    qreal angle = moveToV2.first;
    QVector3D n = moveToV2.second;
    glRotatef(angle, n.x(), n.y(), n.z());

    // draw cylinder if active edge
    qreal radius = m_minRadius + (m_maxRadius * m_edgeScale - m_minRadius) * weight;

    if (m_activeVertices.isEmpty() || (m_activeVertices.contains(v1Id) && m_activeVertices.contains(v2Id))) {

      int slices = static_cast<int>(100 * qSqrt(radius));
      if (slices < 4)
	slices = 4;

      int stacks = 2;

      gluCylinder(qobj, radius / m_scale, radius / m_scale, length, slices, stacks);
    }

    glPopMatrix();
  }
}

void SocialNetWidget::displayLabels()
{
  // draw node labels
  // QFont textFont("Helvetica", 30, 63);
  QFont textFont("Helvetica", 11, 63);

  glDisable(GL_DEPTH_TEST);

  for (int i(0); i < m_selVertices.size(); i++) {

    // retrieve vertex index
    int index = m_selVertices[i];

    // retrieve vertex weight
    qreal weight = m_verticesViews[m_sceneIndex][index].getWeight();

    // retrieve vertices coordinates
    QVector3D v = m_verticesViews[m_sceneIndex][index].getV();
    QVector3D vMV = m_mvMat * v;

    // radius of corresponding node
    qreal radius = (m_minRadius + (m_maxRadius - m_minRadius) * weight) / m_scale;
      
    // set color
    GLfloat *mat_amb_diff = m_verticesColor[m_sceneIndex][index];
    QColor labelColor = QColor::fromRgbF(mat_amb_diff[0], mat_amb_diff[1], mat_amb_diff[2]);
    qglColor(labelColor);
    
    // retrieve label
    QString label = m_verticesViews[m_sceneIndex][index].getLabel();

    // possibly draw it
    if (weight > 0.0 && (m_activeVertices.isEmpty() || m_activeVertices.contains(index))) {
      QPointF pWindow = getWindowCoord(vMV.x() + radius, vMV.y() + radius);
      renderText(pWindow.x(), pWindow.y(), label, textFont);
    }
  }
  
  glEnable(GL_DEPTH_TEST);
}
*/

void SocialNetWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  int currRefHeight = 20;
  
  // filling background
  painter.fillRect(QRect(0, 0, width(), height()), QColor::fromRgb(60, 60, 60, 255));

  // setting font
  QFont textFont;
  textFont.setWeight(63);

  // draw node labels
  painter.setFont(textFont);

  for (int i(0); i < m_selVertices.size(); i++) {

    // retrieve vertex index
    int index = m_selVertices[i];

    // retrieve vertex degree
    qreal strength = m_verticesViews[m_sceneIndex][index].getColorWeight();

    // retrieve vertices coordinates
    QVector3D v = m_verticesViews[m_sceneIndex][index].getV();

    // set color and height
    GLfloat *mat_amb_diff = m_verticesColor[m_sceneIndex][index];
    QColor labelColor = QColor::fromRgbF(mat_amb_diff[0], mat_amb_diff[1], mat_amb_diff[2]);
    painter.setPen(labelColor);

    int height = static_cast<int>(60 * strength) + 10;
    textFont.setPixelSize(height);
    painter.setFont(textFont);

    // retrieve label
    QString label = m_verticesViews[m_sceneIndex][index].getLabel();

    // normalize coordinates
    int normX = normalizeX(v.x());
    int normY = normalizeY(v.y());

    QFontMetrics fm(textFont);
    int width = fm.width(label);

    normX -= width / 2;

    painter.drawText(normX, normY, label);
  }

  // display current season and episode
  QString paddedSeas = QString("%1").arg(m_sceneRefs[m_sceneIndex].first, 2, 10, QChar('0'));
  QString paddedEp = QString("%1").arg(m_sceneRefs[m_sceneIndex].second, 2, 10, QChar('0'));
  QString currRef = "Season " + paddedSeas + ", Ep. " + paddedEp;
  
  textFont.setPixelSize(currRefHeight);

  QFontMetrics fm(textFont);
  int currRefWidth = fm.width(currRef) + fm.width(currRef) / 5;

  painter.setPen(Qt::white);
  painter.setFont(textFont);
  painter.drawText(width() - currRefWidth, height() - currRefHeight, currRef);
}

int SocialNetWidget::normalizeX(qreal x)
{
  qreal normX = static_cast<int>((x * m_scale + 1.0) / 2.0 * width());

  return normX;
}

int SocialNetWidget::normalizeY(qreal y)
{
  qreal normY = height() - static_cast<int>((y * m_scale + 1.0) / 2.0 * height());

  return normY;
}

QSize SocialNetWidget::minimumSizeHint() const
{
  return QSize(300, 300);
}

QSize SocialNetWidget::sizeHint() const
{
  return QSize(600, 600);
}

void SocialNetWidget::mouseMoveEvent(QMouseEvent *event)
{
  int x(event->x());
  int y(event->y());

  if (x >= 0  && x <= width() && y >= 0 && y <= height()) {

    if (event->buttons() == Qt::LeftButton) {

      if (x != m_lastPos.x() || y != m_lastPos.y()) {
	
	// projection of both points on unit sphere
	QVector3D v1 = getArcballVector(m_lastPos.x(), m_lastPos.y());
	QVector3D v2 = getArcballVector(x, y);
      
	// angle and normal vector to resulting vectors
	qreal angle = 4.0 * m_scale * getAngle(v1, v2);
	QVector3D axisInCamCoord = QVector3D::crossProduct(v1, v2);

	// inverse model/view matrix
	bool invertible(1);
	QMatrix4x4 invMvMat = m_mvMat.inverted(&invertible);

	if (invertible) {

	  // rotation axis in object coordinates
	  QVector3D axisInObjCoord = invMvMat * axisInCamCoord;
	
	  // rotating
	  glRotatef(angle, axisInObjCoord.x(), axisInObjCoord.y(), axisInObjCoord.z());
	  m_mvMat.rotate(angle, axisInObjCoord);
	
	  // set current position to last one
	  m_lastPos = event->pos();
	}
      }
    }
    
    else {
      
      // current point
      QPointF pClicked = getObjectCoord(x, y);
      QVector3D vClicked(pClicked.x(), pClicked.y(), 1.0);

      // retrieve clicked node index
      int selVertex = -1;
      qreal minDist(8 * qSqrt(3));
    
      for (int i(0); i < m_selVertices.size(); i++) {

	// current node index
	int index = m_selVertices[i];

	QVector3D v = m_verticesViews[m_sceneIndex][index].getV();
	QString label = m_verticesViews[m_sceneIndex][index].getLabel();
	qreal weight = m_verticesViews[m_sceneIndex][index].getWeight();
	qreal radius = (m_minRadius + (m_maxRadius - m_minRadius) * weight) / m_scale;

	// minimum clickable radius
	if (radius < 0.01)
	  radius = 0.01;

	// corresponding point as projected on screen
	QVector3D vMV = m_mvMat * v;
	
	// distance from viewer
	qreal dist = vClicked.distanceToPoint(vMV);

	// node boundaries
	qreal xInf = vMV.x() - radius;
	qreal xSup = vMV.x() + radius;
	qreal yInf = vMV.y() - radius;
	qreal ySup = vMV.y() + radius;

	// node possibly clicked
	if (vClicked.x() >= xInf && vClicked.x() <= xSup &&
	    vClicked.y() >= yInf && vClicked.y() <= ySup)  {

	  // update clicked node index if necessary
	  if (dist < minDist) {
	    selVertex = index;
	    minDist = dist;
	  }
	}
      }
    
      setActiveVertices(selVertex);
    }

    // updating widget
    updateWidget();
  }
}

void SocialNetWidget::mousePressEvent(QMouseEvent *event)
{
  m_lastPos.setX(event->x());
  m_lastPos.setY(event->y());
}

void SocialNetWidget::wheelEvent(QWheelEvent *event)
{
  int direction = event->angleDelta().y();

  if (direction > 0)
    m_scale += 0.1;
  else if (direction < 0)
    m_scale -= 0.1;
  
  if (m_scale < 0.1)
    m_scale = 0.1;

  // adjustViewPort(width(), height());
  updateWidget();
}

QVector3D SocialNetWidget::getArcballVector(int x, int y)
{
  // retrieve coordinates in objective space
  QPointF objPoint = getObjectCoord(x, y);
  QVector3D p(objPoint.x(), objPoint.y(), 0.0);
  qreal squaredNorm = p.lengthSquared();

  // project objective coordinates onto unit sphere
  if (squaredNorm <= 1.0)
    p.setZ(1.0 * 1.0 - squaredNorm);
  else
    p.normalize();

  return p;
}

qreal SocialNetWidget::getAngle(const QVector3D &v1, const QVector3D &v2)
{
  qreal angle = qAtan2(QVector3D::crossProduct(v1, v2).length(), QVector3D::dotProduct(v1, v2));

  return qRadiansToDegrees(angle);
}

QPointF SocialNetWidget::getObjectCoord(int x, int y)
{
  qreal side = qMin(width(), height());

  qreal wRatio = width() / side;
  qreal hRatio = height() / side;

  QPointF point;
  qreal bound = qSqrt(3.0) + m_maxRadius;
  qreal ratio = 1.0 / m_scale;

  point.setX(ratio * 2.0 * bound * x / side - ratio * bound * wRatio);
  point.setY(ratio * 2.0 * bound * y / side - ratio * bound * hRatio);
  point.setY(-point.y());

  return point;
}

QPointF SocialNetWidget::getWindowCoord(qreal x, qreal y)
{
  qreal side = qMin(width(), height());

  qreal wRatio = width() / side;
  qreal hRatio = height() / side;

  QPointF point;
  qreal bound = qSqrt(3.0) + m_maxRadius;
  qreal ratio = 1.0 / m_scale;

  point.setX((x + ratio * bound * wRatio) * side / (ratio * 2 * bound));
  point.setY((y + ratio * bound * hRatio) * side / (ratio * 2 * bound));
  point.setY(height() - point.y());

  return point;
}

void SocialNetWidget::setVerticesViews(const QVector<QList<Vertex> > &verticesViews)
{
  m_verticesViews = verticesViews;
  m_verticesColor.resize(verticesViews.size());

  /*
  // setting vertices color
  for (int i(0); i < m_verticesColor.size(); i++)
    qDeleteAll(m_verticesColor[i]);
  */

  // sort vertices for each view
  for (int i(0); i < m_verticesViews.size(); i++)
    std::sort(m_verticesViews[i].begin(), m_verticesViews[i].end());

  for (int i(0); i < m_verticesViews.size(); i++) {
    for (int j(0); j < m_verticesViews[i].size(); j++) {

      int com = m_verticesViews[i][j].getCom();
      qreal colorWeight = m_verticesViews[i][j].getWeight();
      GLfloat *mat_amb_diff;

      if (com == 0) {
	// mat_amb_diff = getRGBColor(QColor::fromRgb(100, 100, 100, 255), colorWeight);
	mat_amb_diff = getRGBColor(m_refColor, colorWeight);
      }
      
      else {
	int idxColor = com % m_colors.size();
	// mat_amb_diff = getRGBColor(m_colors[idxColor], colorWeight);
	mat_amb_diff = getRGBColor(m_refColor, colorWeight);
      }

      // if (m_verticesViews[i][j].getLabel() == "Tuco Salamanca")
      // mat_amb_diff = getRGBColor(m_colors[5], colorWeight);

      m_verticesColor[i].push_back(mat_amb_diff);
    }
  }
}

bool SocialNetWidget::compareVertices(const Vertex &v1, const Vertex &v2)
{
  return v1.getWeight() < v2.getWeight();
}

void SocialNetWidget::setEdgesViews(const QVector<QList<Edge> > &edgesViews)
{
  m_edgesViews = edgesViews;
  m_edgesColor.resize(edgesViews.size());

  /*
  // setting edges color
  for (int i(0); i < m_edgesColor.size(); i++)
    qDeleteAll(m_edgesColor[i]);
  */

  for (int i(0); i < m_edgesViews.size(); i++) {
    for (int j(0); j < m_edgesViews[i].size(); j++) {
      qreal colorWeight = m_edgesViews[i][j].getColorWeight();
      GLfloat *mat_amb_diff = getRGBColor(m_refColor, colorWeight);
      m_edgesColor[i].push_back(mat_amb_diff);
    }
  }
}

void SocialNetWidget::setSceneRefs(const QVector<QPair<int, int> > &sceneRefs)
{
  m_sceneRefs = sceneRefs;
}

void SocialNetWidget::setLabelDisplay(bool checked)
{
  m_displayLabels = checked;
  updateWidget();
}

void SocialNetWidget::setFilterWeight(qreal filterWeight)
{
  m_filterWeight = filterWeight;
}

void SocialNetWidget::setActiveVertices(int vId)
{
  m_activeVertices.clear();

  if (vId != -1) {
    
    for (int i(0); i < m_selEdges.size(); i++) {
      int v1Id = m_edgesViews[m_sceneIndex][m_selEdges[i]].getV1Index();
      int v2Id = m_edgesViews[m_sceneIndex][m_selEdges[i]].getV2Index();

      if (v1Id == vId) {
	if (!m_activeVertices.contains(vId))
	  m_activeVertices.push_back(vId);
	m_activeVertices.push_back(v2Id);
      }

      if (v2Id == vId) {
	if (!m_activeVertices.contains(vId))
	  m_activeVertices.push_back(vId);
	m_activeVertices.push_back(v1Id);
      }
    }
  }
}

void SocialNetWidget::setTwoDim(bool twoDim)
{
  m_twoDim = twoDim;

  updateWidget();
}

void SocialNetWidget::updateNetView(int iScene)
{
  m_sceneIndex = iScene;
  emit positionChanged(m_sceneIndex);
}

void SocialNetWidget::updateWidget()
{
  // if (m_twoDim)
  update();
  // else
  // updateGL();
}

/*
void SocialNetWidget::adjustViewPort(int width, int height)
{
  int wSide = width * m_scale;
  int hSide = height * m_scale;
  int side = qMin(wSide, hSide);
  glViewport((width - side) / 2, (height - side) / 2, side, side);
}
*/ 

GLfloat * SocialNetWidget::getRGBColor(const QColor &refColor, qreal weight)
{
  GLfloat *rgb = new GLfloat;
  QColor color;
  qreal h, s, v, hsvA;
  qreal r, g, b, rgbA;

  // retrieving hue, saturation and value components of reference color
  refColor.getHsvF(&h, &s, &v, &hsvA);

  // computing saturation and value components for current vertex/edge
  // as a proportion of reference components
  // s *= weight;
  // v *= weight;
  // v = 1.0 - (1.0 - v) * weight;

  // color to be returned
  color = QColor::fromHsvF(h, s, v, hsvA);
  color.getRgbF(&r, &g, &b, &rgbA);

  rgb[0] = static_cast<GLfloat>(r);
  rgb[1] = static_cast<GLfloat>(g);
  rgb[2] = static_cast<GLfloat>(b);
  rgb[3] = static_cast<GLfloat>(rgbA);

  return rgb;
}

void SocialNetWidget::selectVertices()
{
  m_selVertices.clear();
  
  for (int i(0); i < m_verticesViews[m_sceneIndex].size(); i++) {
    qreal weight = m_verticesViews[m_sceneIndex][i].getWeight();
    if (weight >= m_filterWeight)
      m_selVertices.push_back(i);
  }

  m_selEdges.clear();

  for (int i(0); i < m_edgesViews[m_sceneIndex].size(); i++) {
    int v1Index = m_edgesViews[m_sceneIndex][i].getV1Index();
    int v2Index = m_edgesViews[m_sceneIndex][i].getV2Index();

    if (m_selVertices.contains(v1Index) && m_selVertices.contains(v2Index))
      m_selEdges.push_back(i);
  }

  updateWidget();
}

void SocialNetWidget::selectEdges()
{
  m_selEdges.clear();
  m_selVertices.clear();
  for (int i(0); i < m_edgesViews[m_sceneIndex].size(); i++) {
    qreal weight = m_edgesViews[m_sceneIndex][i].getColorWeight();
    if (weight >= m_filterWeight) {
      m_selEdges.push_back(i);
      int v1Index = m_edgesViews[m_sceneIndex][i].getV1Index();
      int v2Index = m_edgesViews[m_sceneIndex][i].getV2Index();
      if (!m_selVertices.contains(v1Index))
	m_selVertices.push_back(v1Index);
      if (!m_selVertices.contains(v2Index))
	m_selVertices.push_back(v2Index);
    }
  }

  updateWidget();
}

void SocialNetWidget::paintNextScene()
{
  if (m_sceneIndex < m_verticesViews.size() - 1) {
    setSceneIndex(++m_sceneIndex);
    emit positionChanged(m_sceneIndex);
  }

  if (m_sceneIndex == m_verticesViews.size() - 1)
    emit resetPlayer();
}

void SocialNetWidget::setSceneIndex(qint64 sceneIndex)
{
  // QString fName = "animation/images/BB_IMG_" + QString::number(m_sceneIndex) + ".jpg";
  QString fName = "animation/images/GOT_IMG_" + QString::number(m_sceneIndex) + ".jpg";
  // QString fName = "animation/images/HOC_IMG_" + QString::number(m_sceneIndex) + ".jpg";
  QImage img(this->size(), QImage::Format_ARGB32);
  QPainter painter(&img);
  this->render(&painter);

  // if (sceneIndex <= 450)
  // img.save(fName, 0, 100);

  m_sceneIndex = static_cast<int>(sceneIndex);

  selectVertices();

  updateWidget();
}
