/*
** a_movingcamera.cpp
** Cameras that move and related neat stuff
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, self list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, self list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from self software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

/*
== InterpolationPoint: node along a camera's path
==
== args[0] = pitch
== args[1] = time (in octics) to get here from previous node
== args[2] = time (in octics) to stay here before moving to next node
== args[3] = low byte of next node's tid
== args[4] = high byte of next node's tid
*/
class InterpolationPoint : Actor
{
	
	InterpolationPoint	Next;

	bool bVisited;
	
	default
	{
		+NOBLOCKMAP
		+NOGRAVITY
		+DONTSPLASH
		+NOTONAUTOMAP
		RenderStyle "None";
	}
	
	override void BeginPlay ()
	{
		Super.BeginPlay ();
		Next = null;
	}

	override void Tick () {}		// Nodes do no thinking

	void FormChain ()
	{
		let me = self;
		
		while (me != null)
		{
			if (me.bVisited) return;

			me.bVisited = true;

			let iterator = Level.CreateActorIterator(me.args[3] + 256 * me.args[4], "InterpolationPoint");
			me.Next = InterpolationPoint(iterator.Next ());

			if (me.Next == me)	// Don't link to self
				me.Next = InterpolationPoint(iterator.Next ());

			int pt = (me.args[0] << 24) >> 24;	// this is for truncating the value to a byte, presumably because some old WAD needs it...
			me.Pitch = clamp(pt, -89, 89);
				
			if (me.Next == null && (me.args[3] | me.args[4]))
			{
				A_Log("Can't find target for camera node " .. me.tid);
			}

			me = me.Next;
		}
	}

	// Return the node (if any) where a path loops, relative to self one.
	InterpolationPoint ScanForLoop ()
	{
		InterpolationPoint node = self;
		while (node.Next && node.Next != self && node.special1 == 0)
		{
			node.special1 = 1;
			node = node.Next;
		}
		return node.Next == self ? node : null;
	}
	
}

/*
== InterpolationSpecial: Holds a special to execute when a
==  PathFollower reaches an InterpolationPoint of the same TID.
*/

class InterpolationSpecial : Actor
{
	default
	{
		+NOBLOCKMAP
		+NOSECTOR
		+NOGRAVITY
		+DONTSPLASH
		+NOTONAUTOMAP
	}
	
	override void Tick () {}		// Does absolutely nothing itself
	
}

/*
== PathFollower: something that follows a camera path
==		Base class for some moving cameras
==
== args[0] = low byte of first node in path's tid
== args[1] = high byte of first node's tid
== args[2] = bit 0 = follow a linear path (rather than curved)
==			 bit 1 = adjust angle
==			 bit 2 = adjust pitch
==			 bit 3 = aim in direction of motion
==
== Also uses:
==	target = first node in path
==	lastenemy = node prior to first node (if looped)
*/

class PathFollower : Actor 
{
	default
	{
		+NOBLOCKMAP
		+NOSECTOR
		+NOGRAVITY
		+DONTSPLASH
	}
	
	bool bActive, bJustStepped;
	InterpolationPoint PrevNode, CurrNode;
	double Time;		// Runs from 0.0 to 1.0 between CurrNode and CurrNode.Next
	int HoldTime;
	
	// Interpolate between p2 and p3 along a Catmull-Rom spline
	// http://research.microsoft.com/~hollasch/cgindex/curves/catmull-rom.html
	double Splerp (double p1, double p2, double p3, double p4)
	{
		double t = Time;
		double res = 2*p2;
		res += (p3 - p1) * Time;
		t *= Time;
		res += (2*p1 - 5*p2 + 4*p3 - p4) * t;
		t *= Time;
		res += (3*p2 - 3*p3 + p4 - p1) * t;
		return 0.5 * res;
	}

	// Linearly interpolate between p1 and p2
	double Lerp (double p1, double p2)
	{
		return p1 + Time * (p2 - p1);
	}

	override void BeginPlay ()
	{
		Super.BeginPlay ();
		PrevNode = CurrNode = null;
		bActive = false;
	}

	override void PostBeginPlay ()
	{
		// Find first node of path
		let iterator = Level.CreateActorIterator(args[0] + 256 * args[1], "InterpolationPoint");
		let node = InterpolationPoint(iterator.Next ());
		InterpolationPoint prevnode;

		target = node;

		if (node == null)
		{
			A_Log ("PathFollower " .. tid .. ": Can't find interpolation point " .. args[0] + 256 * args[1] .. "\n");
			return;
		}

		// Verify the path has enough nodes
		node.FormChain ();
		if (args[2] & 1)
		{	// linear path; need 2 nodes
			if (node.Next == null)
			{
				A_Log ("PathFollower " .. tid .. ": Path needs at least 2 nodes\n");
				return;
			}
			lastenemy = null;
		}
		else
		{	// spline path; need 4 nodes
			if (node.Next == null ||
				node.Next.Next == null ||
				node.Next.Next.Next == null)
			{
				A_Log ("PathFollower " .. tid .. ": Path needs at least 4 nodes\n");
				return;
			}
			// If the first node is in a loop, we can start there.
			// Otherwise, we need to start at the second node in the path.
			prevnode = node.ScanForLoop ();
			if (prevnode == null || prevnode.Next != node)
			{
				lastenemy = target;
				target = node.Next;
			}
			else
			{
				lastenemy = prevnode;
			}
		}
	}

	override void Deactivate (Actor activator)
	{
		bActive = false;
	}

	override void Activate (Actor activator)
	{
		if (!bActive)
		{
			CurrNode = InterpolationPoint(target);
			PrevNode = InterpolationPoint(lastenemy);

			if (CurrNode != null)
			{
				NewNode ();
				SetOrigin (CurrNode.Pos, false);
				Time = 0.;
				HoldTime = 0;
				bJustStepped = true;
				bActive = true;
			}
		}
	}

	override void Tick ()
	{
		if (!bActive)
			return;

		if (bJustStepped)
		{
			bJustStepped = false;
			if (CurrNode.args[2])
			{
				HoldTime = Level.maptime + CurrNode.args[2] * TICRATE / 8;
				SetXYZ(CurrNode.Pos);
			}
		}

		if (HoldTime > Level.maptime)
			return;

		// Splines must have a previous node.
		if (PrevNode == null && !(args[2] & 1))
		{
			bActive = false;
			return;
		}

		// All paths must have a current node.
		if (CurrNode.Next == null)
		{
			bActive = false;
			return;
		}

		if (Interpolate ())
		{
			Time += (8. / (max(1, CurrNode.args[1]) * TICRATE));
			if (Time > 1.)
			{
				Time -= 1.;
				bJustStepped = true;
				PrevNode = CurrNode;
				CurrNode = CurrNode.Next;
				if (CurrNode != null)
					NewNode ();
				if (CurrNode == null || CurrNode.Next == null)
					Deactivate (self);
				if ((args[2] & 1) == 0 && CurrNode.Next.Next == null)
					Deactivate (self);
			}
		}
	}

	void NewNode ()
	{
		let iterator = Level.CreateActorIterator(CurrNode.tid, "InterpolationSpecial");
		InterpolationSpecial spec;

		while ( (spec = InterpolationSpecial(iterator.Next ())) )
		{
			Level.ExecuteSpecial(spec.special, null, null, false, spec.args[0], spec.args[1], spec.args[2], spec.args[3], spec.args[4]);
		}
	}

	virtual bool Interpolate ()
	{
		Vector3 dpos = (0, 0, 0);
		LinkContext ctx;

		if ((args[2] & 8) && Time > 0)
		{
			dpos = Pos;
		}

		if (CurrNode.Next==null) return false;

		UnlinkFromWorld (ctx);
		Vector3 newpos;
		if (args[2] & 1)
		{	// linear
			newpos.X = Lerp(CurrNode.pos.X, CurrNode.Next.pos.X);
			newpos.Y = Lerp(CurrNode.pos.Y, CurrNode.Next.pos.Y);
			newpos.Z = Lerp(CurrNode.pos.Z, CurrNode.Next.pos.Z);
		}
		else
		{	// spline
			if (CurrNode.Next.Next==null) return false;

			newpos.X = Splerp(PrevNode.pos.X, CurrNode.pos.X, CurrNode.Next.pos.X, CurrNode.Next.Next.pos.X);
			newpos.Y = Splerp(PrevNode.pos.Y, CurrNode.pos.Y, CurrNode.Next.pos.Y, CurrNode.Next.Next.pos.Y);
			newpos.Z = Splerp(PrevNode.pos.Z, CurrNode.pos.Z, CurrNode.Next.pos.Z, CurrNode.Next.Next.pos.Z);
		}
		SetXYZ(newpos);
		LinkToWorld (ctx);

		if (args[2] & 6)
		{
			if (args[2] & 8)
			{
				if (args[2] & 1)
				{ // linear
					dpos.X = CurrNode.Next.pos.X - CurrNode.pos.X;
					dpos.Y = CurrNode.Next.pos.Y - CurrNode.pos.Y;
					dpos.Z = CurrNode.Next.pos.Z - CurrNode.pos.Z;
				}
				else if (Time > 0)
				{ // spline
					dpos = newpos - dpos;
				}
				else
				{
					int realarg = args[2];
					args[2] &= ~(2|4|8);
					Time += 0.1;
					dpos = newpos;
					Interpolate ();
					Time -= 0.1;
					args[2] = realarg;
					dpos = newpos - dpos;
					newpos -= dpos;
					SetXYZ(newpos);
				}
				if (args[2] & 2)
				{ // adjust yaw
					Angle = VectorAngle(dpos.X, dpos.Y);
				}
				if (args[2] & 4)
				{ // adjust pitch;
					double dist = dpos.XY.Length();
					Pitch = dist != 0 ? VectorAngle(dist, -dpos.Z) : 0.;
				}
			}
			else
			{
				if (args[2] & 2)
				{ // interpolate angle
					double angle1 = Normalize180(CurrNode.angle);
					double angle2 = angle1 + deltaangle(angle1, CurrNode.Next.angle);
					angle = Lerp(angle1, angle2);
				}
				if (args[2] & 1)
				{ // linear
					if (args[2] & 4)
					{ // interpolate pitch
						Pitch = Lerp(CurrNode.Pitch, CurrNode.Next.Pitch);
					}
				}
				else
				{ // spline
					if (args[2] & 4)
					{ // interpolate pitch
						Pitch = Splerp(PrevNode.Pitch, CurrNode.Pitch,
							CurrNode.Next.Pitch, CurrNode.Next.Next.Pitch);
					}
				}
			}
		}

		return true;
	}
	
}

/*
== ActorMover: Moves any actor along a camera path
==
== Same as PathFollower, except
== args[2], bit 7: make nonsolid
== args[3] = tid of thing to move
==
== also uses:
==	tracer = thing to move
*/

class ActorMover : PathFollower
{
	override void BeginPlay()
	{
		ChangeStatNum(STAT_ACTORMOVER);
	}

	override void PostBeginPlay ()
	{
		Super.PostBeginPlay ();

		let iterator = Level.CreateActorIterator(args[3]);
		tracer = iterator.Next ();

		if (tracer == null)
		{
			A_Log("ActorMover " .. tid .. ": Can't find target " .. args[3] .. "\n");
		}
		else
		{
			special1 = tracer.bNoGravity + (tracer.bNoBlockmap<<1) + (tracer.bSolid<<2) + (tracer.bInvulnerable<<4) + (tracer.bDormant<<8);
		}
	}

	override bool Interpolate ()
	{
		if (tracer == null)
			return true;

		if (Super.Interpolate ())
		{
			double savedz = tracer.pos.Z;
			tracer.SetZ(pos.Z);
			if (!tracer.TryMove (Pos.XY, true))
			{
				tracer.SetZ(savedz);
				return false;
			}

			if (args[2] & 2)
				tracer.angle = angle;
			if (args[2] & 4)
				tracer.Pitch = Pitch;

			return true;
		}
		return false;
	}

	override void Activate (Actor activator)
	{
		if (tracer == null || bActive)
			return;

		Super.Activate (activator);
		let tracer = self.tracer;
		special1 = tracer.bNoGravity + (tracer.bNoBlockmap<<1) + (tracer.bSolid<<2) + (tracer.bInvulnerable<<4) + (tracer.bDormant<<8);
		tracer.bNoGravity = true;
		if (args[2] & 128)
		{
			LinkContext ctx;
			tracer.UnlinkFromWorld (ctx);
			tracer.bNoBlockmap = true;
			tracer.bSolid = false;
			tracer.LinkToWorld (ctx);
		}
		if (tracer.bIsMonster)
		{
			tracer.bInvulnerable = true;
			tracer.bDormant = true;
		}
		// Don't let the renderer interpolate between the actor's
		// old position and its new position.
		Interpolate ();
		tracer.ClearInterpolation();
	}

	override void Deactivate (Actor activator)
	{
		if (bActive)
		{
			Super.Deactivate (activator);
			let tracer = self.tracer;
			if (tracer != null)
			{
				LinkContext ctx;
				tracer.UnlinkFromWorld (ctx);
				tracer.bNoGravity = (special1 & 1);
				tracer.bNoBlockmap = !!(special1 & 2);
				tracer.bSolid = !!(special1 & 4);
				tracer.bInvulnerable = !!(special1 & 8);
				tracer.bDormant = !!(special1 & 16);
				tracer.LinkToWorld (ctx);
			}
		}
	}
}

/*
== MovingCamera: Moves any actor along a camera path
==
== Same as PathFollower, except
== args[3] = tid of thing to look at (0 if none)
==
== Also uses:
==	tracer = thing to look at
*/

class MovingCamera : PathFollower
{
	Actor activator;
	default
	{
		CameraHeight 0;
	}
	
	override void PostBeginPlay ()
	{
		Super.PostBeginPlay ();

		Activator = null;
		if (args[3] != 0)
		{
			let iterator = Level.CreateActorIterator(args[3]);
			tracer = iterator.Next ();
			if (tracer == null)
			{
				A_Log("MovingCamera " .. tid .. ": Can't find thing " .. args[3] .. "\n");
			}
		}
	}

	override bool Interpolate ()
	{
		if (tracer == null)
			return Super.Interpolate ();

		if (Super.Interpolate ())
		{
			angle = AngleTo(tracer, true);

			if (args[2] & 4)
			{ // Also aim camera's pitch;
				Vector3 diff = Pos - tracer.Pos - (0, 0, tracer.Height / 2);
				double dist = diff.XY.Length();
				Pitch = dist != 0 ? VectorAngle(dist, diff.Z) : 0.;
			}

			return true;
		}
		return false;
	}
	
}
