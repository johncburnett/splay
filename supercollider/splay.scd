(
/*
* splay
*/

//--------------------------------------------------------------
// init

~len = 5 * 60; // gesture length
~channels = 2;

~splay = false;
~alwaysSplay = true;
~splayOdds = 3;

// drone
~dur = 32.0;    // duration of each swell
~space = 4.0;   // space between swell onsets
~n = (~len / ~space * 1.593625498).asInteger; // number of drone iterations

// grains
~gDur = 4;

// melt
~meltAmp = 0.4;
~chorusAmp = 0.0;

// osc
~ip = "127.0.0.1";
~port = 7771;
~osc = NetAddr.new(~ip, ~port);

// graphics
~opacity = 0.04;
~rs = 0.3;

// recording
~doRecord = false;

// server
s = Server.local;
o = s.options;
o.memSize_(2e6); // 2GB of RAM
o.numOutputBusChannels = ~channels;

s.waitForBoot({

	//--------------------------------------------------------------
	// buffers

	~cwd = thisProcess.nowExecutingPath.dirname;
	~audio = (~cwd +/+ "samples/*").pathMatch.collect { |file| Buffer.read(s, file) };

	~nSamples = ~audio.size;
	~sampleQueue = (0..(~nSamples-1)).scramble;
	~si = ~sampleQueue.last;

	s.sync;

	//--------------------------------------------------------------
	// granulator

	~grains = JGrain(s)
	.buf_(~audio[~si])
	.rate_(1.2)
	.pos_(0)
	.dur_(~gDur)
	.pan_(0)
	.delta_( { 1.001.rrand(1.01) * 0.8 } )
	.amp_(0.0)
	.start(20);

	s.sync;

	//--------------------------------------------------------------
	// synths

	SynthDef(\looper_1ch, { |bufnum=0, rate=1.0, pan=0, amp=0.0, send=0.0|
		var in, mix;
		in = PlayBuf.ar(1, bufnum, rate, loop:1);
		mix = Pan2.ar(in, pan);
		Out.ar(10, mix * send); 	// to processing
		Out.ar(20, mix * amp);	    // to mixer
	}).add;

	SynthDef(\looper_1ch_alt, { |bufnum=0, rate=1.0, amp=0.0, send=0.0, dur=8, pos=0|
		var in, mix, env;
		in = PlayBuf.ar(1, bufnum, rate, loop:1, startPos:pos).dup;
		mix = in * (SinOsc.kr(0.0625) + 1 * 0.5);
		mix = Pan2.ar(mix, SinOsc.kr(0.0625*0.5));
		Out.ar(10, mix * send); 	// to processing
		Out.ar(20, mix * amp);	    // to mixer
	}).add;

	SynthDef(\chorus, { |amp=1.0, predelay=0.08, speed=0.05, depth=0.1, ph_diff=0.5|
		var in1, in2, mod1, mod2, sig1, sig2, mix, numDelays = 12;
		in1 = In.ar(10, 1) * numDelays.reciprocal;
		in2 = In.ar(11, 1) * numDelays.reciprocal;
		mod1 = Array.fill(numDelays, { |i|
			LFPar.kr(speed * rrand(0.94, 1.06), ph_diff * i, depth, predelay);
		});
		mod2 = Array.fill(numDelays, { |i|
			LFPar.kr(speed * rrand(0.94, 1.06), ph_diff * i, depth, predelay);
		});
		sig1 = DelayC.ar(in1, 0.5, mod1);
		sig2 = DelayC.ar(in2, 0.5, mod2);

		mix = [sig1, sig2];
		Out.ar(20, mix);
	}).add;

	SynthDef(\melt,{ |amp=0.5, mod_scale=1.0, dur=12|
		var in, melt, d1, d2, pan, mix, env;
		in = In.ar(10, 2);

		melt = Array.fill(2, { |i|
			Array.fill(12, {
				BPF.ar(
					CombC.ar(
						in[i],
						15.0,
						LinLin.kr(
							SinOsc.kr(rrand(1/100, 1/150) * mod_scale),
							-1.0,
							1.0,
							rrand(8.0, 12.0) * mod_scale,
							rrand(16.0, 24.0) * mod_scale ),
						10) * mod_scale,
					rrand(200, 10000),
					0.3)
			} ).sum / 2;
		});

		pan = SinOsc.kr(0.125) * 0.6;
		d1 = Pan2.ar(melt[0], pan);
		d2 = Pan2.ar(melt[1], -1.0*pan);
		mix = ([d1, d2]*2) * amp;
		Out.ar(20, mix);
	}).add;

	SynthDef(\phaseMod, { |amp=0.8, carrier=1000.0, mod=400.0, pan=0|
		var phaseEnv, ampEnv, pmOsc, mix;
		phaseEnv = Line.ar(0.0, 10.0.rand, 8);
		ampEnv   = EnvGen.ar(Env.linen(~dur*0.25, ~dur*0.5, ~dur*0.25), doneAction:2);
		pmOsc    = PMOsc.ar(carrier, mod, phaseEnv, mul:0.1);
		mix      = Pan2.ar(pmOsc * ampEnv, pan);
		Out.ar(30, mix * amp);
	}).add;

	SynthDef(\mixer,{ |amp = 1.0|
		var in1, in2, mix;
		in1 = In.ar(20, ~channels);
		in2 = In.ar(30, ~channels);
		mix = (in1+in2) * amp;
		Out.ar(0, mix);
		SendPeakRMS.kr(in1 * amp, 20, 3, '/rms');
	}).add;

	s.sync;

	//--------------------------------------------------------------
	// utilities

	~fadein = { |synth, amp=0.8, len=10.0, res=1000|
		fork{
			res.do({ |i|
				synth.set(\send, (i/res*amp));
				(len/res).wait;
			});
		};
	};

	~fadeout = { |synth, amp=0.8, len=10.0, res=1000|
		fork{
			res.do({ |i|
				synth.set(\send, amp - (i/res*amp));
				(len/res).wait;
			});
		};
	};

	~interpOSC = { |msg, lo, hi, len, res=10000|
		fork{
			var dt = len/res;
			res.do({ |i|
				var val, spec;
				val = i/res;
				spec = ControlSpec(lo, hi, \lin, 0.00001);
				~osc.sendMsg(msg, spec.map(val));
				dt.wait;
			});
		};
	};

	~record = {
		s.sync;
		s.prepareForRecord;
		"Recording...".postln;
		fork{
			s.record;
			(~len + ~dur).wait;
			s.stopRecording;
		};
		"Finshed Recording".postln;
	};

	//--------------------------------------------------------------
	// drone methods

	~fifthGen = { |fundamental=200.0, numFreqs=4, freqRatio=1.5, ampRatio=0.8, amp=0.2|
		var freqs, amps, times;
		freqs = Array(numFreqs).add(fundamental);
		amps  = Array(numFreqs).add(amp);
		times = Array(numFreqs).add(exprand(0.5, 1.5));
		for(1, numFreqs, { |i|
			freqs.add(freqs[i-1] * freqRatio);
			amps.add(amps[i-1] * ampRatio);
			times.add(exprand(10.5, 11.5));
		});
		[freqs, amps, times];
	};

	~spatializer = { |freqAmpSet, numChannels=2|
		var spread = Array.fill(3, {
			Array.fill(numChannels, { Array(freqAmpSet[0].size / numChannels) })
		});
		for(0, freqAmpSet[0].size-1, { |i|
			spread[0][i % numChannels].add(freqAmpSet[0][i]);
			spread[1][i % numChannels].add(freqAmpSet[1][i]);
			spread[2][i % numChannels].add(freqAmpSet[2][i]);
		});
		spread;
	};

	~numVoices = 15;
	~resonance = ~fifthGen.(
		fundamental: 7.75, // fundamental of A
		numFreqs:((~numVoices*1.5).asInteger)
	);
	~resonance = [
		~resonance[0][4..14],
		~resonance[1][0..12],
		~resonance[2][0..12]
	];

	// gaussian envelope
	~gauss = { |x, w, iphase=0| exp(squared(x - iphase) / (-2.0 * squared(w))) };
	~curve = ( all {: ~gauss.(x, 0.15, 3.1415 / 4), x <- (0, 0.01..1.6) } ).normalize;
	~gaussEnv = { |x| ~curve[ (x * 159).asInteger ] };

	~droneGen = {
		fork{
			for(0, ~n-1, { |i|
				var scale = 1.0 / ~n * i;
				var index = (~gaussEnv.(scale) * ~resonance[0].size).asInteger;
				if(i < (~n/2), { scale = 1.0 / ~n * i }, { scale = 1.0 / ~n * ((~n/2) - (i - (~n/2))) });

				Synth(\phaseMod, [
					\carrier, ~resonance[0][0..index].choose,
					\mod,     (~resonance[0][0..index].choose + rrand(-5.0, 5.0)),
					\pan,     1.0.rand2,
					\amp,     (-4).dbamp,
				]);

				~space = 4 - (scale * 3);
				~space.wait;
			});
		};
	};

	~subDrone = {
		fork{
			~sn = (~n/2).asInteger;
			for(0, ~sn-1, { |i|
				var scale = 1.0 / ~sn * i;
				var index = (~gaussEnv.(scale) * ~resonance[0].size * 0.3).asInteger;
				if(i < (~sn/2), { scale = 1.0 / ~sn * i }, { scale = 1.0 / ~sn * ((~sn/2) - (i - (~sn/2))) });

				Synth(\phaseMod, [
					\carrier, ~resonance[0][0..index].choose,
					\mod,     (~resonance[0][0..index].choose + rrand(-5.0, 5.0)),
					\pan,     1.0.rand2,
					\amp,     (-4).dbamp,
				]);

				~space = 4 - (scale * 3);
				~space.wait;
			});
		};
	};

	//--------------------------------------------------------------
	// grain methods

	~rs1 = ControlSpec(1.0, 0.01, \lin, 0.0, 1.0);
	~rs2 = ControlSpec(1.0, 2,  \lin, 0.0, 1.0);
	~ds1 = ControlSpec(~gDur, 0.0004, \exp, 0.0, 1.0);
	~ds2 = ControlSpec(~gDur, 0.004,  \exp, 0.0, 1.0);

	~fadeinGrains = { |a=2, len=10.0, res=500|
		fork{
			res.do({ |i|
				~grains.amp_(i/res*a);
				(len/res).wait;
			});
		};
	};

	~fadeoutGrains = { |a=2, len=10.0, res=500|
		fork{
			res.do({ |i|
				~grains.amp_(a - (i/res*a));
				(len/res).wait;
			});
		};
	};

	s.sync;

	//--------------------------------------------------------------
	// routing

	~mix = Synth(\mixer);

	~chorus = Synth(\chorus, [\amp, ~chorusAmp]);
	~melt = Synth(\melt, [\amp, ~meltAmp]);

	~samplers = all {: Synth(\looper_1ch_alt, [\bufnum, ~audio[n].bufnum, \amp, 0]), n <- (0..(~nSamples-1)) };

	s.sync;

	//--------------------------------------------------------------
	// main

	fork{
		~fadeinGrains.(len:120);
		loop{
			var dt = ~len / 1000;
			~si = ~sampleQueue.pop;
			~fadein.(~samplers[~si], amp:0.7, len:(~len/4.0));
			~fadeinGrains.(len:(~len/8.0));
			~interpOSC.('/time', 0, 1000, len:~len);
			~grains.buf_(~audio[~si]);
			~droneGen.();

			~interpOSC.('/rs', 0.1, ~rs, len:(~len/2.0));
			for(0, 499, { |i|
				var val, r0, ri, d0, di;
				val = i / 1000.0;
				r0 = ~rs1.map(val);
				ri = ~rs2.map(val);
				d0 = ~ds1.map(val/2);
				di = ~ds2.map(val/2);
				~grains.rate_( {(r0).rrand(ri)} );
				~grains.delta_( {(d0).rrand(di)} * 0.5 );
				~grains.pan_( { val.rand2 * 2.0 } );
				dt.wait;
			});

			~interpOSC.('/rs', ~rs, 0.1, len:(~len/2.0));
			if(~alwaysSplay.not, { ~splay = (false!(~splayOdds-1) ++ true).choose; }, { ~splay = true; });

			for(499, 0, { |i|
				var val, r0, ri, d0, di;
				val = i / 1000.0;
				r0 = ~rs1.map(val);
				ri = ~rs2.map(val);
				d0 = ~ds1.map(val/2);
				di = ~ds2.map(val/2);
				~grains.rate_( {(r0).rrand(ri)} );
				~grains.delta_( {(d0).rrand(di)} * 0.5 );
				~grains.pan_( { val.rand2 * 2.0 } );
				dt.wait;
			});

			if(~sampleQueue.size == 0, { ~sampleQueue = (0..(~nSamples-1)).scramble });
			~fadeout.(~samplers[~si], amp:0.7, len:(~len/8.0));
			~fadeoutGrains.(len:(~len/4.0));

			if(~splay, {
				~subDrone.();
				~interpOSC.('/time', 1000, 2500, len:(~len/4));
				~osc.sendMsg('/distortOn', 1);
				~osc.sendMsg('/scanOn', 1);
				~interpOSC.('/camZ', 200.0, 135.0, len:(~len/4.0));
				~interpOSC.('/distort', 0.0, 0.5, len:(~len/4.0));
				~interpOSC.('/rs', 0.1, ~rs, len:(~len/4.0));
				~interpOSC.('/scanRate', 0.0, 0.3, len:(~len/8.0));

				(~len/8.0).wait;

				~interpOSC.('/opacity', ~opacity, 0.0, len:(~len/8.0));

				(~len/8.0).wait;

				~interpOSC.('/time', 500, 0, len:(~len/4));
				~osc.sendMsg('/swap', 1);
				~interpOSC.('/camZ', 135.0, 200.0, len:(~len/4.0));
				~interpOSC.('/distort', 0.5, 0.0, len:(~len/4.0));
				~interpOSC.('/rs', ~rs, 0.1, len:(~len/4.0));
				~interpOSC.('/scanHeight', 330.0, 0.0, len:(~len/4.0));
				~interpOSC.('/scanRate', 0.3, 0.0, len:(~len/8.0));
				~interpOSC.('/opacity', 0.0, ~opacity, len:(~len/16.0));

				(~len/8.0).wait;

				~interpOSC.('/scanRate', 0.3, 0.0, len:(~len/4.0));

				(~len/8.0).wait;

				~osc.sendMsg('/distortOff', 1);
				~osc.sendMsg('/scanOff', 1);
			},{
				~osc.sendMsg('/swap', 1);
			});
		};
	};

	//--------------------------------------------------------------
	// osc

	OSCFunc({ |msg, time|
		~osc.sendMsg('/rms', msg[4]);
	},'/rms', s.addr);

	~osc.sendMsg('/camX', 2.0);
	~osc.sendMsg('/camY', 100.0);
	~osc.sendMsg('/camZ', 200.0);
	~osc.sendMsg('/opacity', ~opacity);
	~osc.sendMsg('rs', 0.01);

	//--------------------------------------------------------------
	// recording

	if(~doRecord, { ~record.() });
});
)
