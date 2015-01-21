/*
 * DriveMovement.h
 *
 *  Created on: 17 Jan 2015
 *      Author: David
 */

#ifndef DRIVEMOVEMENT_H_
#define DRIVEMOVEMENT_H_

// Set the following nonzero to cache the square of startSpeedTimesCdivA, at the cost of 64 bytes of memory per DDA
#define CACHE_startSpeedTimesCdivAsquared	0

class DDA;

// Struct for passing parameters to the DriveMovement Prepare methods
struct PrepParams
{
	float decelStartDistance;
	uint32_t startSpeedTimesCdivA;
	uint32_t topSpeedTimesCdivA;
	uint32_t decelStartClocks;
	uint32_t topSpeedTimesCdivAPlusDecelStartClocks;
	uint32_t accelClocksMinusAccelDistanceTimesCdivTopSpeed;
	float compFactor;
};

// This class describes a single movement of one drive
class DriveMovement
{
public:
	uint32_t CalcNextStepTimeCartesian(size_t drive);
	uint32_t CalcNextStepTimeDelta(const DDA &dda, size_t drive);
	void PrepareCartesianAxis(const DDA& dda, const PrepParams& params, size_t drive);
	void PrepareDeltaAxis(const DDA& dda, const PrepParams& params, size_t drive);
	void PrepareExtruder(const DDA& dda, const PrepParams& params, size_t drive);
	void ReduceSpeedCartesian(float inverseSpeedfactor, bool isNearEndstop);
	void DebugPrint(char c, bool withDelta) const;

	static uint32_t isqrt(uint64_t num);

	// Parameters common to Cartesian, delta and extruder moves
	// These values don't depend on how the move is executed, so  are set by Init()
	uint32_t totalSteps;								// total number of steps for this move
	bool moving;										// true if this drive moves in this move, if false then all other values are don't cares
	bool direction;										// true=forwards, false=backwards
	bool stepError;										// for debugging

	// The following only need to be stored per-drive if we are supporting elasticity compensation
	uint32_t startSpeedTimesCdivA;
#if CACHE_startSpeedTimesCdivAsquared
	uint64_t startSpeedTimesCdivAsquared;
#endif
	int32_t accelClocksMinusAccelDistanceTimesCdivTopSpeed;		// this one can be negative
	uint32_t topSpeedTimesCdivAPlusDecelStartClocks;
	uint64_t twoDistanceToStopTimesCsquaredDivA;

	// Parameters unique to a style of move (Cartesian, delta or extruder). Currently, extruders and Cartesian moves use the same parameters.
	union MoveParams
	{
		struct CartesianParameters						// Parameters for Cartesian and extruder movement, including extruder pre-compensation
		{
			// The following don't depend on how the move is executed, so they could be set up in Init()
			uint64_t twoCsquaredTimesMmPerStepDivA;		// 2 * clock^2 * mmPerStepInHyperCuboidSpace / acceleration

			// The following depend on how the move is executed, so they must be set up in Prepare()
			uint32_t accelStopStep;						// the first step number at which we are no longer accelerating
			uint32_t decelStartStep;					// the first step number at which we are decelerating
			uint32_t reverseStartStep;					// the first step number for which we need to reverse direction to to elastic compensation
			uint32_t mmPerStepTimesCdivtopSpeed;		// mmPerStepInHyperCuboidSpace * clock / topSpeed

			// The following only need to be stored per-drive if we are supporting elasticity compensation
			int64_t fourMaxStepDistanceMinusTwoDistanceToStopTimesCsquaredDivA;		// this one can be negative
		} cart;

		struct DeltaParameters							// Parameters for delta movement
		{
			// The following don't depend on how the move is executed, so they can be set up in Init
			uint32_t reverseStartStep;
			uint32_t hmz0sK;							// the starting step position less the starting Z height, multiplied by the Z movement fraction and K
			int32_t minusAaPlusBbTimesKs;
			int64_t dSquaredMinusAsquaredMinusBsquaredTimesKsquaredSsquared;
			uint64_t twoCsquaredTimesMmPerStepDivAK;	// this could be stored in the DDA if all towers use the same steps/mm

			// The following depend on how the move is executed, so they must be set up in Prepare()
			uint32_t accelStopDsK;
			uint32_t decelStartDsK;
			uint32_t mmPerStepTimesCdivtopSpeedK;
		} delta;
	} mp;

	// These values change as the step is executed
	uint32_t nextStep;									// number of steps already done
	uint32_t nextStepTime;								// how many clocks after the start of this move the next step is due

	static const uint32_t NoStepTime = 0xFFFFFFFF;		// value to indicate that no further steps are needed when calculating the next step time
	static const uint32_t K1 = 1024;					// a power of 2 used to multiply the value mmPerStepTimesCdivtopSpeed to reduce rounding errors
	static const uint32_t K2 = 1024;					// a power of 2 used in delta calculations to reduce rounding errors
	static const int32_t Kc = 4096;						// a power of 2 for scaling the Z movement fraction
};

#endif /* DRIVEMOVEMENT_H_ */
