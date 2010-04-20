#ifndef GALIPEUR_H_
#define GALIPEUR_H_

///@file

#include <vector>
#include "robot.h"


/** @brief Rob'Otter robot, Galipeur
 *
 * A triangular holonomic robot.
 *
 * Galipeur shares code with RBasic but has its own (re)implementation for
 * better flexibility, since it may (will?) evolves in a different way.
 */
class Galipeur: public Robot
{
  friend class LuaGalipeur;
public:
  Galipeur(btScalar m);
  virtual ~Galipeur();

  virtual void addToWorld(Physics *physics);
  virtual void removeFromWorld();

  void setColor(const Color4 &color) { color_ = color; }

  /** @brief Draw the robot
   *
   * Assumes that the robot is not rotated (robot's Z axis aligned with world's
   * Z axis).
   */
  virtual void draw();

  virtual const btTransform &getTrans() const { return body_->getCenterOfMassTransform(); }
  virtual void setTrans(const btTransform &tr) { body_->setCenterOfMassTransform(tr); }

  /** @brief Asserv step
   *
   * Go in position and/or turn according to current target.
   */
  virtual void asserv();

  /** @name Strategy functions and orders
   */
  //@{
  const btVector2 get_xy() const { return this->getPos(); }
  const btVector2 get_v()  const { return body_->getLinearVelocity(); }
  btScalar get_a() const
  {
    btScalar a, p, r;
    this->getRot().getEulerYPR(a,p,r);
    return a;
  }
  btScalar get_av() const { return body_->getAngularVelocity().getZ(); }

  void order_xy(btVector2 xy, bool rel=false);
  void order_a(btScalar a, bool rel=false);
  void order_xya(btVector2 xy, btScalar a, bool rel=false);
  void order_stop() { stopped_ = false; }

  bool is_waiting() { return in_position_; }

  btScalar test_sensor(unsigned int i);
  //@}

  /** @name Asserv configuration.
   */
  //@{
  void set_speed_xy(btScalar v, btScalar a) { ramp_xy_.var_v = v; ramp_xy_.var_a = a; }
  void set_speed_a (btScalar v, btScalar a) { ramp_a_ .var_v = v; ramp_a_ .var_a = a; }
  void set_threshold_xy(btScalar t) { threshold_xy_ = t; }
  void set_threshold_a (btScalar t) { threshold_a_  = t; }
  //@}

  /** @brief Implementation of the aversive quadramp filter.
   *
   * It is not an exact equivalent of the aversive module, it is only intended
   * to have the same behavior.
   *
   * Deceleration distance can be computed as following:
   * \f{eqnarray*}{
   *   v_{dec}(t) & = & v_0 - a t \\
   *   x_{dec}(t) & = & v_0 t - \frac12 a t^2 \\
   *   t_{dec}    & = & \frac{v_0}a \\
   *   d_{dec}    & = & x_{dec}(t_{dec}) = \frac12 \frac{v_0^2}a
   * \f}
   */
  class Quadramp
  {
  public:
    Quadramp(): var_v(0), var_a(0), cur_v_(0) {}
    /// Reset current values.
    void reset(btScalar v=0) { cur_v_ = v; }
    /** @brief Feed the filter and return the new velocity.
     *
     * @param d   actual distance to target
     * @param dt  elapsed time since the last step
     */
    btScalar step(btScalar d, btScalar dt);

  public:
    btScalar var_v, var_a;
  private:
    btScalar cur_v_;
  };

  /** @name Shape constants
   */
  //@{
  static const btScalar z_mass;   ///< Z position of the center of mass
  static const btScalar ground_clearance;

  static const btScalar height;   ///< body height
  static const btScalar side;     ///< triangle side half size
  static const btScalar w_block;  ///< motor block half width (small side)
  static const btScalar r_wheel;  ///< wheel radius
  static const btScalar h_wheel;  ///< wheel height (when laid flat)

  static const btScalar d_side;   ///< distance center/triangle side
  static const btScalar d_wheel;  ///< distance center/wheel side
  static const btScalar a_side;   ///< center angle of triangle side
  static const btScalar a_wheel;  ///< center angle of wheel side
  static const btScalar radius;   ///< outer circle radius
  //@}

protected:
  btRigidBody *body_;

  Color4 color_;

  /// Sharp positions
  std::vector<btTransform> sharps_trans_;

  /** @name Orders.
   */
  //@{

  bool in_position_; ///< \e true if target has been reached
  bool stopped_;     ///< \e true if asserv is not running

  btVector2 target_xy_;
  btScalar  target_a_;

  //@}

  Quadramp ramp_xy_, ramp_a_;
  btScalar ramp_last_t_; ///< Last update time of ramps.
  btScalar threshold_xy_, threshold_a_;

  void set_v(btVector2 vxy);
  void set_av(btScalar v);

private:
  static SmartPtr<btCompoundShape> shape_;
  static btConvexHullShape body_shape_;
  static btBoxShape wheel_shape_;
  /** @brief Display list shared by all instances
   * @todo Created display list is not deleted.
   */
  static GLuint dl_id_static_;
};


#endif
