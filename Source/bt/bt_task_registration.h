// General enemy tasks
REGISTER_TASK(ChooseWaypoint)
REGISTER_TASK(TargetPlayer)
REGISTER_TASK(TargetNearPlayer)
REGISTER_TASK(MoveToPosition)
REGISTER_TASK(MoveToEntity)
REGISTER_TASK(CombatMovement)
REGISTER_TASK(RotateToFace)
REGISTER_TASK(Backstep)
REGISTER_TASK(MoveBackwards)
REGISTER_TASK(Healing)
REGISTER_TASK(Hitstunned)
REGISTER_TASK(Stunned)
REGISTER_TASK(EnemyDeath)
REGISTER_TASK(Dash)
REGISTER_TASK(Dodge)
REGISTER_TASK(WarpDodge)
REGISTER_TASK(FMODSetGlobalRTPC)

// Melee enemy tasks
REGISTER_TASK(ThrustAttack)
REGISTER_TASK(CircularAttack)
REGISTER_TASK(DashStrike)

// Ranged enemy tasks
REGISTER_TASK(RangedAttack)
REGISTER_TASK(LookAround)

// Gard Tasks
REGISTER_TASK(GardAreaAttack)
REGISTER_TASK(GardRangedAttack)
REGISTER_TASK(GardSweepAttack)
REGISTER_TASK(GardJumpAttack)
REGISTER_TASK(GardAreaDelay)
REGISTER_TASK(GardDeath)

// Cygnus Tasks
REGISTER_TASK(CygnusAreaDelay)
REGISTER_TASK(CygnusRangedAttack)
REGISTER_TASK(CygnusMeleeAttack)
REGISTER_TASK(CygnusDodge)
REGISTER_TASK(CygnusWalkAggressive)
REGISTER_TASK(CygnusDeath)
REGISTER_TASK(CygnusTimeReversal)

// Generic Tasks
REGISTER_TASK(Wait)
REGISTER_TASK(SetBlackboardBool)
REGISTER_TASK(SetBlackboardFloat)
REGISTER_TASK(SetBlackboardInt)
REGISTER_TASK(DecrementBlackboardInt)

// Debugging tasks
REGISTER_TASK(Success)
REGISTER_TASK(Failure)
REGISTER_TASK(InProgress)
