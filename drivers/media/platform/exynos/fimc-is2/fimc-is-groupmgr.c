 total count of connected ISDB-Tsb
			channels.</para>
		<para>Possible values: 1 .. 13</para>
		<para>Note: This value cannot be determined by an automatic channel search.</para>
	</section>
	<section id="isdb-hierq-layers">
		<title><constant>DTV-ISDBT-LAYER*</constant> parameters</title>
		<para>ISDB-T channels can be coded hierarchically. As opposed to DVB-T in
			ISDB-T hierarchical layers can be decoded simultaneously. For that
			reason a ISDB-T demodulator has 3 Viterbi and 3 Reed-Solomon decoders.</para>
		<para>ISDB-T has 3 hierarchical layers which each can use a part of the
			available segments. The total number of segments over all layers has
			to 13 in ISDB-T.</para>
		<para>There are 3 parameter sets, for Layers A, B and C.</para>
		<section id="DTV-ISDBT-LAYER-ENABLED">
			<title><constant>DTV_ISDBT_LAYER_ENABLED</constant></title>
			<para>Hierarchical reception in ISDB-T is achieved by enabling or disabling
				layers in the decoding process. Setting all bits of
				<constant>DTV_ISDBT_LAYER_ENABLED</constant> to '1' forces all layers (if applicable) to be
				demodulated. This is the default.</para>
			<para>If the channel is in the partial reception mode
				(<constant>DTV_ISDBT_PARTIAL_RECEPTION</constant> = 1) the central segment can be decoded
				independently of the other 12 segments. In that mode layer A has to
				have a <constant>SEGMENT_COUNT</constant> of 1.</para>
			<para>In ISDB-Tsb only layer A is used, it can be 1 or 3 in ISDB-Tsb
				according to <constant>DTV_ISDBT_PARTIAL_RECEPTION</constant>. <constant>SEGMENT_COUNT</constant> must be filled
				accordingly.</para>
			<para>Possible values: 0x1, 0x2, 0x4 (|-able)</para>
			<para><constant>DTV_ISDBT_LAYER_ENABLED[0:0]</constant> - layer A</para>
			<para><constant>DTV_ISDBT_LAYER_ENABLED[1:1]</constant> - layer B</para>
			<para><constant>DTV_ISDBT_LAYER_ENABLED[2:2]</constant> - layer C</para>
			<para><constant>DTV_ISDBT_LAYER_ENABLED[31:3]</constant> unused</para>
		</section>
		<section id="DTV-ISDBT-LAYER-FEC">
			<title><constant>DTV_ISDBT_LAYER*_FEC</constant></title>
			<para>Possible values: <constant>FEC_AUTO</constant>, <constant>FEC_1_2</constant>, <constant>FEC_2_3</constant>, <constant>FEC_3_4</constant>, <constant>FEC_5_6</constant>, <constant>FEC_7_8</constant></para>
		</section>
		<section id="DTV-ISDBT-LAYER-MODULATION">
			<title><constant>DTV_ISDBT_LAYER*_MODULATION</constant></title>
			<para>Possible values: <constant>QAM_AUTO</constant>, QP<constant>SK, QAM_16</constant>, <constant>QAM_64</constant>, <constant>DQPSK</constant></para>
			<para>Note: If layer C is <constant>DQPSK</constant> layer B has to be <constant>DQPSK</constant>. If layer B is <constant>DQPSK</constant>
				and <constant>DTV_ISDBT_PARTIAL_RECEPTION</constant>=0 layer has to be <constant>DQPSK</constant>.</para>
		</section>
		<section id="DTV-ISDBT-LAYER-SEGMENT-COUNT">
			<title><constant>DTV_ISDBT_LAYER*_SEGMENT_COUNT</constant></title>
			<para>Possible values: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1 (AUTO)</para>
			<para>Note: Truth table for <constant>DTV_ISDBT_SOUND_BROADCASTING</constant> and
				<constant>DTV_ISDBT_PARTIAL_RECEPTION</constant> and <constant>LAYER</constant>*_SEGMENT_COUNT</para>
			<informaltable id="isdbt-layer_seg-cnt-table">
				<tgroup cols="6">
					<tbody>
						<row>
							<entry>PR</entry>
							<entry>SB</entry>
							<entry>Layer A width</entry>
							<entry>Layer B width</entry>
							<entry>Layer C width</entry>
							<entry>total width</entry>
						</row>
						<row>
							<entry>0</entry>
							<entry>0</entry>
							<entry>1 .. 13</entry>
							<entry>1 .. 13</entry>
							<entry>1 .. 13</entry>
							<entry>13</entry>
						</row>
						<row>
							<entry>1</entry>
							<entry>0</entry>
							<entry>1</entry>
							<entry>1 .. 13</entry>
							<entry>1 .. 13</entry>
							<entry>13</entry>
						</row>
						<row>
							<entry>0</entry>
							<entry>1</entry>
							<entry>1</entry>
							<entry>0</entry>
							<entry>0</entry>
							<entry>1</entry>
						</row>
						<row>
							<entry>1</entry>
							<entry>1</entry>
							<entry>1</entry>
							<entry>2</entry>
							<entry>0</entry>
							<entry>13</entry>
						</row>
					</tbody>
				</tgroup>
			</informaltable>
		</section>
		<section id="DTV-ISDBT-LAYER-TIME-INTERLEAVING">
			<title><constant>DTV_ISDBT_LAYER*_TIME_INTERLEAVING</constant></title>
			<para>Valid values: 0, 1, 2, 4, -1 (AUTO)</para>
			<para>when DTV_ISDBT_SOUND_BROADCASTING is active, value 8 is also valid.</para>
			<para>Note: The real time interleaving length depends on the mode (fft-size). The values
				here are referring to what can be found in the TMCC-structure, as shown in the table below.</para>
			<informaltable id="isdbt-layer-interleaving-table">
				<tgroup cols="4" align="center">
					<tbody>
						<row>
							<entry>DTV_ISDBT_LAYER*_TIME_INTERLEAVING</entry>
							<entry>Mode 1 (2K FFT)</entry>
							<entry>Mode 2 (4K FFT)</entry>
							<entry>Mode 3 (8K FFT)</entry>
						</row>
						<row>
							<entry>0</entry>
							<entry>0</entry>
							<entry>0</entry>
							<entry>0</entry>
						</row>
						<row>
							<entry>1</entry>
							<entry>4</entry>
							<entry>2</entry>
							<entry>1</entry>
						</row>
						<row>
							<entry>2</entry>
							<entry>8</entry>
							<entry>4</entry>
							<entry>2</entry>
						</row>
						<row>
							<entry>4</entry>
							<entry>16</entry>
							<entry>8</entry>
							<entry>4</entry>
						</row>
					</tbody>
				</tgroup>
			</informaltable>
		</section>
		<section id="DTV-ATSCMH-FIC-VER">
			<title><constant>DTV_ATSCMH_FIC_VER</constant></title>
			<para>Version number of the FIC (Fast Information Channel) signaling data.</para>
			<para>FIC is used for relaying information to allow rapid service acquisition by the receiver.</para>
			<para>Possible values: 0, 1, 2, 3, ..., 30, 31</para>
		</section>
		<section id="DTV-ATSCMH-PARADE-ID">
			<title><constant>DTV_ATSCMH_PARADE_ID</constant></title>
			<para>Parade identification number</para>
			<para>A parade is a collection of up to eight MH groups, conveying one or two ensembles.</para>
			<para>Possible values: 0, 1, 2, 3, ..., 126, 127</para>
		</section>
		<section id="DTV-ATSCMH-NOG">
			<title><constant>DTV_ATSCMH_NOG</constant></title>
			<para>Number of MH groups per MH subframe for a designated parade.</para>
			<para>Possible values: 1, 2, 3, 4, 5, 6, 7, 8</para>
		</section>
		<section id="DTV-ATSCMH-TNOG">
			<title><constant>DTV_ATSCMH_TNOG</constant></title>
			<para>Total number of MH groups including all MH groups belonging to all MH parades in one MH subframe.</para>
			<para>Possible values: 0, 1, 2, 3, ..., 30, 31</para>
		</section>
		<section id="DTV-ATSCMH-SGN">
			<title><constant>DTV_ATSCMH_SGN</constant></title>
			<para>Start group number.</para>
			<para>Possible values: 0, 1, 2, 3, ..., 14, 15</para>
		</section>
		<section id="DTV-ATSCMH-PRC">
			<title><constant>DTV_ATSCMH_PRC</constant></title>
			<para>Parade repetition cycle.</para>
			<para>Possible values: 1, 2, 3, 4, 5, 6, 7, 8</para>
		</section>
		<section id="DTV-ATSCMH-RS-FRAME-MODE">
			<title><constant>DTV_ATSCMH_RS_FRAME_MODE</constant></title>
			<para>Reed Solomon (RS) frame mode.</para>
			<para>Possible values are:</para>
<table pgwide="1" frame="none" id="atscmh-rs-frame-mode">
    <title>enum atscmh_rs_frame_mode</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="ATSCMH-RSFRAME-PRI-ONLY"><constant>ATSCMH_RSFRAME_PRI_ONLY</constant></entry>
	    <entry>Single Frame: There is only a primary RS Frame for all
		Group Regions.</entry>
	</row><row>
	    <entry id="ATSCMH-RSFRAME-PRI-SEC"><constant>ATSCMH_RSFRAME_PRI_SEC</constant></entry>
	    <entry>Dual Frame: There are two separate RS Frames: Primary RS
		Frame for Group Region A and B and Secondary RS Frame for Group
		Region C and D.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
		<section id="DTV-ATSCMH-RS-FRAME-ENSEMBLE">
			<title><constant>DTV_ATSCMH_RS_FRAME_ENSEMBLE</constant></title>
			<para>Reed Solomon(RS) frame ensemble.</para>
			<para>Possible values are:</para>
<table pgwide="1" frame="none" id="atscmh-rs-frame-ensemble">
    <title>enum atscmh_rs_frame_ensemble</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="ATSCMH-RSFRAME-ENS-PRI"><constant>ATSCMH_RSFRAME_ENS_PRI</constant></entry>
	    <entry>Primary Ensemble.</entry>
	</row><row>
	    <entry id="ATSCMH-RSFRAME-ENS-SEC"><constant>AATSCMH_RSFRAME_PRI_SEC</constant></entry>
	    <entry>Secondary Ensemble.</entry>
	</row><row>
	    <entry id="ATSCMH-RSFRAME-RES"><constant>AATSCMH_RSFRAME_RES</constant></entry>
	    <entry>Reserved. Shouldn't be used.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
		<section id="DTV-ATSCMH-RS-CODE-MODE-PRI">
			<title><constant>DTV_ATSCMH_RS_CODE_MODE_PRI</constant></title>
			<para>Reed Solomon (RS) code mode (primary).</para>
			<para>Possible values are:</para>
<table pgwide="1" frame="none" id="atscmh-rs-code-mode">
    <title>enum atscmh_rs_code_mode</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="ATSCMH-RSCODE-211-187"><constant>ATSCMH_RSCODE_211_187</constant></entry>
	    <entry>Reed Solomon code (211,187).</entry>
	</row><row>
	    <entry id="ATSCMH-RSCODE-223-187"><constant>ATSCMH_RSCODE_223_187</constant></entry>
	    <entry>Reed Solomon code (223,187).</entry>
	</row><row>
	    <entry id="ATSCMH-RSCODE-235-187"><constant>ATSCMH_RSCODE_235_187</constant></entry>
	    <entry>Reed Solomon code (235,187).</entry>
	</row><row>
	    <entry id="ATSCMH-RSCODE-RES"><constant>ATSCMH_RSCODE_RES</constant></entry>
	    <entry>Reserved. Shouldn't be used.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
		<section id="DTV-ATSCMH-RS-CODE-MODE-SEC">
			<title><constant>DTV_ATSCMH_RS_CODE_MODE_SEC</constant></title>
			<para>Reed Solomon (RS) code mode (secondary).</para>
			<para>Possible values are the same as documented on
			    &atscmh-rs-code-mode;:</para>
		</section>
		<section id="DTV-ATSCMH-SCCC-BLOCK-MODE">
			<title><constant>DTV_ATSCMH_SCCC_BLOCK_MODE</constant></title>
			<para>Series Concatenated Convolutional Code Block Mode.</para>
			<para>Possible values are:</para>
<table pgwide="1" frame="none" id="atscmh-sccc-block-mode">
    <title>enum atscmh_scc_block_mode</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="ATSCMH-SCCC-BLK-SEP"><constant>ATSCMH_SCCC_BLK_SEP</constant></entry>
	    <entry>Separate SCCC: the SCCC outer code mode shall be set independently
		for each Group Region (A, B, C, D)</entry>
	</row><row>
	    <entry id="ATSCMH-SCCC-BLK-COMB"><constant>ATSCMH_SCCC_BLK_COMB</constant></entry>
	    <entry>Combined SCCC: all four Regions shall have the same SCCC outer
		code mode.</entry>
	</row><row>
	    <entry id="ATSCMH-SCCC-BLK-RES"><constant>ATSCMH_SCCC_BLK_RES</constant></entry>
	    <entry>Reserved. Shouldn't be used.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
		<section id="DTV-ATSCMH-SCCC-CODE-MODE-A">
			<title><constant>DTV_ATSCMH_SCCC_CODE_MODE_A</constant></title>
			<para>Series Concatenated Convolutional Code Rate.</para>
			<para>Possible values are:</para>
<table pgwide="1" frame="none" id="atscmh-sccc-code-mode">
    <title>enum atscmh_sccc_code_mode</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="ATSCMH-SCCC-CODE-HLF"><constant>ATSCMH_SCCC_CODE_HLF</constant></entry>
	    <entry>The outer code rate of a SCCC Block is 1/2 rate.</entry>
	</row><row>
	    <entry id="ATSCMH-SCCC-CODE-QTR"><constant>ATSCMH_SCCC_CODE_QTR</constant></entry>
	    <entry>The outer code rate of a SCCC Block is 1/4 rate.</entry>
	</row><row>
	    <entry id="ATSCMH-SCCC-CODE-RES"><constant>ATSCMH_SCCC_CODE_RES</constant></entry>
	    <entry>to be documented.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
		<section id="DTV-ATSCMH-SCCC-CODE-MODE-B">
			<title><constant>DTV_ATSCMH_SCCC_CODE_MODE_B</constant></title>
			<para>Series Concatenated Convolutional Code Rate.</para>
			<para>Possible values are the same as documented on
			    &atscmh-sccc-code-mode;.</para>
		</section>
		<section id="DTV-ATSCMH-SCCC-CODE-MODE-C">
			<title><constant>DTV_ATSCMH_SCCC_CODE_MODE_C</constant></title>
			<para>Series Concatenated Convolutional Code Rate.</para>
			<para>Possible values are the same as documented on
			    &atscmh-sccc-code-mode;.</para>
		</section>
		<section id="DTV-ATSCMH-SCCC-CODE-MODE-D">
			<title><constant>DTV_ATSCMH_SCCC_CODE_MODE_D</constant></title>
			<para>Series Concatenated Convolutional Code Rate.</para>
			<para>Possible values are the same as documented on
			    &atscmh-sccc-code-mode;.</para>
		</section>
	</section>
	<section id="DTV-API-VERSION">
	<title><constant>DTV_API_VERSION</constant></title>
	<para>Returns the major/minor version of the DVB API</para>
	</section>
	<section id="DTV-CODE-RATE-HP">
	<title><constant>DTV_CODE_RATE_HP</constant></title>
	<para>Used on terrestrial transmissions.  The acceptable values are
	    the ones described at &fe-transmit-mode-t;.
	</para>
	</section>
	<section id="DTV-CODE-RATE-LP">
	<title><constant>DTV_CODE_RATE_LP</constant></title>
	<para>Used on terrestrial transmissions. The acceptable values are
	    the ones described at &fe-transmit-mode-t;.
	</para>

	</section>

	<section id="DTV-GUARD-INTERVAL">
		<title><constant>DTV_GUARD_INTERVAL</constant></title>

		<para>Possible values are:</para>

<section id="fe-guard-interval-t">
<title>Modulation guard interval</title>

<table pgwide="1" frame="none" id="fe-guard-interval">
    <title>enum fe_guard_interval</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="GUARD-INTERVAL-AUTO"><constant>GUARD_INTERVAL_AUTO</constant></entry>
	    <entry>Autodetect the guard interval</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-1-128"><constant>GUARD_INTERVAL_1_128</constant></entry>
	    <entry>Guard interval 1/128</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-1-32"><constant>GUARD_INTERVAL_1_32</constant></entry>
	    <entry>Guard interval 1/32</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-1-16"><constant>GUARD_INTERVAL_1_16</constant></entry>
	    <entry>Guard interval 1/16</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-1-8"><constant>GUARD_INTERVAL_1_8</constant></entry>
	    <entry>Guard interval 1/8</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-1-4"><constant>GUARD_INTERVAL_1_4</constant></entry>
	    <entry>Guard interval 1/4</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-19-128"><constant>GUARD_INTERVAL_19_128</constant></entry>
	    <entry>Guard interval 19/128</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-19-256"><constant>GUARD_INTERVAL_19_256</constant></entry>
	    <entry>Guard interval 19/256</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-PN420"><constant>GUARD_INTERVAL_PN420</constant></entry>
	    <entry>PN length 420 (1/4)</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-PN595"><constant>GUARD_INTERVAL_PN595</constant></entry>
	    <entry>PN length 595 (1/6)</entry>
	</row><row>
	    <entry id="GUARD-INTERVAL-PN945"><constant>GUARD_INTERVAL_PN945</constant></entry>
	    <entry>PN length 945 (1/9)</entry>
	</row>
        </tbody>
    </tgroup>
</table>

		<para>Notes:</para>
		<para>1) If <constant>DTV_GUARD_INTERVAL</constant> is set the <constant>GUARD_INTERVAL_AUTO</constant> the hardware will
			try to find the correct guard interval (if capable) and will use TMCC to fill
			in the missing parameters.</para>
		<para>2) Intervals 1/128, 19/128 and 19/256 are used only for DVB-T2 at present</para>
		<para>3) DTMB specifies PN420, PN595 and PN945.</para>
</section>
	</section>
	<section id="DTV-TRANSMISSION-MODE">
		<title><constant>DTV_TRANSMISSION_MODE</constant></title>

		<para>Specifies the number of carriers used by the standard.
		    This is used only on OFTM-based standards, e. g.
		    DVB-T/T2, ISDB-T, DTMB</para>

<section id="fe-transmit-mode-t">
<title>enum fe_transmit_mode: Number of carriers per channel</title>

<table pgwide="1" frame="none" id="fe-transmit-mode">
    <title>enum fe_transmit_mode</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="TRANSMISSION-MODE-AUTO"><constant>TRANSMISSION_MODE_AUTO</constant></entry>
	    <entry>Autodetect transmission mode. The hardware will try to find
		the correct FFT-size (if capable) to fill in the missing
		parameters.</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-1K"><constant>TRANSMISSION_MODE_1K</constant></entry>
	    <entry>Transmission mode 1K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-2K"><constant>TRANSMISSION_MODE_2K</constant></entry>
	    <entry>Transmission mode 2K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-8K"><constant>TRANSMISSION_MODE_8K</constant></entry>
	    <entry>Transmission mode 8K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-4K"><constant>TRANSMISSION_MODE_4K</constant></entry>
	    <entry>Transmission mode 4K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-16K"><constant>TRANSMISSION_MODE_16K</constant></entry>
	    <entry>Transmission mode 16K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-32K"><constant>TRANSMISSION_MODE_32K</constant></entry>
	    <entry>Transmission mode 32K</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-C1"><constant>TRANSMISSION_MODE_C1</constant></entry>
	    <entry>Single Carrier (C=1) transmission mode (DTMB)</entry>
	</row><row>
	    <entry id="TRANSMISSION-MODE-C3780"><constant>TRANSMISSION_MODE_C3780</constant></entry>
	    <entry>Multi Carrier (C=3780) transmission mode (DTMB)</entry>
	</row>
        </tbody>
    </tgroup>
</table>


		<para>Notes:</para>
		<para>1) ISDB-T supports three carrier/symbol-size: 8K, 4K, 2K. It is called
			'mode' in the standard: Mode 1 is 2K, mode 2 is 4K, mode 3 is 8K</para>

		<para>2) If <constant>DTV_TRANSMISSION_MODE</constant> is set the <constant>TRANSMISSION_MODE_AUTO</constant> the
			hardware will try to find the correct FFT-size (if capable) and will
			use TMCC to fill in the missing parameters.</para>
		<para>3) DVB-T specifies 2K and 8K as valid sizes.</para>
		<para>4) DVB-T2 specifies 1K, 2K, 4K, 8K, 16K and 32K.</para>
		<para>5) DTMB specifies C1 and C3780.</para>
</section>
	</section>
	<section id="DTV-HIERARCHY">
	<title><constant>DTV_HIERARCHY</constant></title>
	<para>Frontend hierarchy</para>


<section id="fe-hierarchy-t">
<title>Frontend hierarchy</title>

<table pgwide="1" frame="none" id="fe-hierarchy">
    <title>enum fe_hierarchy</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	     <entry id="HIERARCHY-NONE"><constant>HIERARCHY_NONE</constant></entry>
	    <entry>No hierarchy</entry>
	</row><row>
	     <entry id="HIERARCHY-AUTO"><constant>HIERARCHY_AUTO</constant></entry>
	    <entry>Autodetect hierarchy (if supported)</entry>
	</row><row>
	     <entry id="HIERARCHY-1"><constant>HIERARCHY_1</constant></entry>
	    <entry>Hierarchy 1</entry>
	</row><row>
	     <entry id="HIERARCHY-2"><constant>HIERARCHY_2</constant></entry>
	    <entry>Hierarchy 2</entry>
	</row><row>
	     <entry id="HIERARCHY-4"><constant>HIERARCHY_4</constant></entry>
	    <entry>Hierarchy 4</entry>
	</row>
        </tbody>
    </tgroup>
</table>
</section>

	</section>
	<section id="DTV-STREAM-ID">
	<title><constant>DTV_STREAM_ID</constant></title>
	<para>DVB-S2, DVB-T2 and ISDB-S support the transmission of several
	      streams on a single transport stream.
	      This property enables the DVB driver to handle substream filtering,
	      when supported by the hardware.
	      By default, substream filtering is disabled.
	</para><para>
	      For DVB-S2 and DVB-T2, the valid substream id range is from 0 to 255.
	</para><para>
	      For ISDB, the valid substream id range is from 1 to 65535.
	</para><para>
	      To disable it, you should use the special macro NO_STREAM_ID_FILTER.
	</para><para>
	      Note: any value outside the id range also disables filtering.
	</para>
	</section>
	<section id="DTV-DVBT2-PLP-ID-LEGACY">
		<title><constant>DTV_DVBT2_PLP_ID_LEGACY</constant></title>
		<para>Obsolete, replaced with DTV_STREAM_ID.</para>
	</section>
	<section id="DTV-ENUM-DELSYS">
		<title><constant>DTV_ENUM_DELSYS</constant></title>
		<para>A Multi standard frontend needs to advertise the delivery systems provided.
			Applications need to enumerate the provided delivery systems, before using
			any other operation with the frontend. Prior to it's introduction,
			FE_GET_INFO was used to determine a frontend type. A frontend which
			provides more than a single delivery system, FE_GET_INFO doesn't help much.
			Applications which intends to use a multistandard frontend must enumerate
			the delivery systems associated with it, rather than trying to use
			FE_GET_INFO. In the case of a legacy frontend, the result is just the same
			as with FE_GET_INFO, but in a more structured format </para>
	</section>
	<section id="DTV-INTERLEAVING">
	<title><constant>DTV_INTERLEAVING</constant></title>

<para>Time interleaving to be used. Currently, used only on DTMB.</para>

<table pgwide="1" frame="none" id="fe-interleaving">
    <title>enum fe_interleaving</title>
    <tgroup cols="2">
	&cs-def;
	<thead>
	<row>
	    <entry>ID</entry>
	    <entry>Description</entry>
	</row>
	</thead>
	<tbody valign="top">
	<row>
	    <entry id="INTERLEAVING-NONE"><constant>INTERLEAVING_NONE</constant></entry>
	    <entry>No interleaving.</entry>
	</row><row>
	    <entry id="INTERLEAVING-AUTO"><constant>INTERLEAVING_AUTO</constant></entry>
	    <entry>Auto-detect interleaving.</entry>
	</row><row>
	    <entry id="INTERLEAVING-240"><constant>INTERLEAVING_240</constant></entry>
	    <entry>Interleaving of 240 symbols.</entry>
	</row><row>
	    <entry id="INTERLEAVING-720"><constant>INTERLEAVING_720</constant></entry>
	    <entry>Interleaving of 720 symbols.</entry>
	</row>
        </tbody>
    </tgroup>
</table>

	</section>
	<section id="DTV-LNA">
	<title><constant>DTV_LNA</constant></title>
	<para>Low-noise amplifier.</para>
	<para>Hardware might offer controllable LNA which can be set manually
		using that parameter. Usually LNA could be found only from
		terrestrial devices if at all.</para>
	<para>Possible values: 0, 1, LNA_AUTO</para>
	<para>0, LNA off</para>
	<para>1, LNA on</para>
	<para>use the special macro LNA_AUTO to set LNA auto</para>
	</section>
</section>

	<section id="frontend-stat-properties">
	<title>Frontend statistics indicators</title>
	<para>The values are returned via <constant>dtv_property.stat</constant>.
	      If the property is supported, <constant>dtv_property.stat.len</constant> is bigger than zero.</para>
	<para>For most delivery systems, <constant>dtv_property.stat.len</constant>
	      will be 1 if the stats is supported, and the properties will
	      return a single value for each parameter.</para>
	<para>It should be noted, however, that new OFDM delivery systems
	      like ISDB can use different modulation types for each group of
	      carriers. On such standards, up to 3 groups of statistics can be
	      provided, and <constant>dtv_property.stat.len</constant> is updated
	      to reflect the "global" metrics, plus one metric per each carrier
	      group (called "layer" on ISDB).</para>
	<para>So, in order to be consistent with other delivery systems, the first
	      value at <link linkend="dtv-stats"><constant>dtv_property.stat.dtv_stats</constant></link>
	      array refers to the global metric. The other elements of the array
	      represent each layer, starting from layer A(index 1),
	      layer B (index 2) and so on.</para>
	<para>The number of filled elements are stored at <constant>dtv_property.stat.len</constant>.</para>
	<para>Each element of the <constant>dtv_property.stat.dtv_stats</constant> array consists on two elements:</para>
	<itemizedlist mark='opencircle'>
		<listitem><para><constant>svalue</constant> or <constant>uvalue</constant>, where
			<constant>svalue</constant> is for signed values of the measure (dB measures)
			and <constant>uvalue</constant> is for unsigned values (counters, relative scale)</para></listitem>
		<listitem><para><constant>scale</constant> - Scale for the value. It can be:</para>
			<itemizedlist mark='bullet' id="fecap-scale-params">
				<listitem id="FE-SCALE-NOT-AVAILABLE"><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - The parameter is supported by the frontend, but it was not possible to collect it (could be a transitory or permanent condition)</para></listitem>
				<listitem id="FE-SCALE-DECIBEL"><para><constant>FE_SCALE_DECIBEL</constant> - parameter is a signed value, measured in 1/1000 dB</para></listitem>
				<listitem id="FE-SCALE-RELATIVE"><para><constant>FE_SCALE_RELATIVE</constant> - parameter is a unsigned value, where 0 means 0% and 65535 means 100%.</para></listitem>
				<listitem id="FE-SCALE-COUNTER"><para><constant>FE_SCALE_COUNTER</constant> - parameter is a unsigned value that counts the occurrence of an event, like bit error, block error, or lapsed time.</para></listitem>
			</itemizedlist>
		</listitem>
	</itemizedlist>
	<section id="DTV-STAT-SIGNAL-STRENGTH">
		<title><constant>DTV_STAT_SIGNAL_STRENGTH</constant></title>
		<para>Indicates the signal strength level at the analog part of the tuner or of the demod.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_DECIBEL</constant> - signal strength is in 0.001 dBm units, power measured in miliwatts. This value is generally negative.</para></listitem>
			<listitem><para><constant>FE_SCALE_RELATIVE</constant> - The frontend provides a 0% to 100% measurement for power (actually, 0 to 65535).</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-CNR">
		<title><constant>DTV_STAT_CNR</constant></title>
		<para>Indicates the Signal to Noise ratio for the main carrier.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_DECIBEL</constant> - Signal/Noise ratio is in 0.001 dB units.</para></listitem>
			<listitem><para><constant>FE_SCALE_RELATIVE</constant> - The frontend provides a 0% to 100% measurement for Signal/Noise (actually, 0 to 65535).</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-PRE-ERROR-BIT-COUNT">
		<title><constant>DTV_STAT_PRE_ERROR_BIT_COUNT</constant></title>
		<para>Measures the number of bit errors before the forward error correction (FEC) on the inner coding block (before Viterbi, LDPC or other inner code).</para>
		<para>This measure is taken during the same interval as <constant>DTV_STAT_PRE_TOTAL_BIT_COUNT</constant>.</para>
		<para>In order to get the BER (Bit Error Rate) measurement, it should be divided by
		<link linkend="DTV-STAT-PRE-TOTAL-BIT-COUNT"><constant>DTV_STAT_PRE_TOTAL_BIT_COUNT</constant></link>.</para>
		<para>This measurement is monotonically increased, as the frontend gets more bit count measurements.
		      The frontend may reset it when a channel/transponder is tuned.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of error bits counted before the inner coding.</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-PRE-TOTAL-BIT-COUNT">
		<title><constant>DTV_STAT_PRE_TOTAL_BIT_COUNT</constant></title>
		<para>Measures the amount of bits received before the inner code block, during the same period as
		<link linkend="DTV-STAT-PRE-ERROR-BIT-COUNT"><constant>DTV_STAT_PRE_ERROR_BIT_COUNT</constant></link> measurement was taken.</para>
		<para>It should be noted that this measurement can be smaller than the total amount of bits on the transport stream,
		      as the frontend may need to manually restart the measurement, losing some data between each measurement interval.</para>
		<para>This measurement is monotonically increased, as the frontend gets more bit count measurements.
		      The frontend may reset it when a channel/transponder is tuned.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of bits counted while measuring
				 <link linkend="DTV-STAT-PRE-ERROR-BIT-COUNT"><constant>DTV_STAT_PRE_ERROR_BIT_COUNT</constant></link>.</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-POST-ERROR-BIT-COUNT">
		<title><constant>DTV_STAT_POST_ERROR_BIT_COUNT</constant></title>
		<para>Measures the number of bit errors after the forward error correction (FEC) done by inner code block (after Viterbi, LDPC or other inner code).</para>
		<para>This measure is taken during the same interval as <constant>DTV_STAT_POST_TOTAL_BIT_COUNT</constant>.</para>
		<para>In order to get the BER (Bit Error Rate) measurement, it should be divided by
		<link linkend="DTV-STAT-POST-TOTAL-BIT-COUNT"><constant>DTV_STAT_POST_TOTAL_BIT_COUNT</constant></link>.</para>
		<para>This measurement is monotonically increased, as the frontend gets more bit count measurements.
		      The frontend may reset it when a channel/transponder is tuned.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of error bits counted after the inner coding.</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-POST-TOTAL-BIT-COUNT">
		<title><constant>DTV_STAT_POST_TOTAL_BIT_COUNT</constant></title>
		<para>Measures the amount of bits received after the inner coding, during the same period as
		<link linkend="DTV-STAT-POST-ERROR-BIT-COUNT"><constant>DTV_STAT_POST_ERROR_BIT_COUNT</constant></link> measurement was taken.</para>
		<para>It should be noted that this measurement can be smaller than the total amount of bits on the transport stream,
		      as the frontend may need to manually restart the measurement, losing some data between each measurement interval.</para>
		<para>This measurement is monotonically increased, as the frontend gets more bit count measurements.
		      The frontend may reset it when a channel/transponder is tuned.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of bits counted while measuring
				 <link linkend="DTV-STAT-POST-ERROR-BIT-COUNT"><constant>DTV_STAT_POST_ERROR_BIT_COUNT</constant></link>.</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-ERROR-BLOCK-COUNT">
		<title><constant>DTV_STAT_ERROR_BLOCK_COUNT</constant></title>
		<para>Measures the number of block errors after the outer forward error correction coding (after Reed-Solomon or other outer code).</para>
		<para>This measurement is monotonically increased, as the frontend gets more bit count measurements.
		      The frontend may reset it when a channel/transponder is tuned.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of error blocks counted after the outer coding.</para></listitem>
		</itemizedlist>
	</section>
	<section id="DTV-STAT-TOTAL-BLOCK-COUNT">
		<title><constant>DTV-STAT_TOTAL_BLOCK_COUNT</constant></title>
		<para>Measures the total number of blocks received during the same period as
		<link linkend="DTV-STAT-ERROR-BLOCK-COUNT"><constant>DTV_STAT_ERROR_BLOCK_COUNT</constant></link> measurement was taken.</para>
		<para>It can be used to calculate the PER indicator, by dividing
		<link linkend="DTV-STAT-ERROR-BLOCK-COUNT"><constant>DTV_STAT_ERROR_BLOCK_COUNT</constant></link>
		by <link linkend="DTV-STAT-TOTAL-BLOCK-COUNT"><constant>DTV-STAT-TOTAL-BLOCK-COUNT</constant></link>.</para>
		<para>Possible scales for this metric are:</para>
		<itemizedlist mark='bullet'>
			<listitem><para><constant>FE_SCALE_NOT_AVAILABLE</constant> - it failed to measure it, or the measurement was not complete yet.</para></listitem>
			<listitem><para><constant>FE_SCALE_COUNTER</constant> - Number of blocks counted while measuring
			<link linkend="DTV-STAT-ERROR-BLOCK-COUNT"><constant>DTV_STAT_ERROR_BLOCK_COUNT</constant></link>.</para></listitem>
		</itemizedlist>
	</section>
	</section>

	<section id="frontend-property-terrestrial-systems">
	<title>Properties used on terrestrial delivery systems</title>
		<section id="dvbt-params">
			<title>DVB-T delivery system</title>
			<para>The following parameters are valid for DVB-T:</para>
			<itemizedlist mark='opencircle'>
				<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CODE-RATE-HP"><constant>DTV_CODE_RATE_HP</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CODE-RATE-LP"><constant>DTV_CODE_RATE_LP</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-GUARD-INTERVAL"><constant>DTV_GUARD_INTERVAL</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TRANSMISSION-MODE"><constant>DTV_TRANSMISSION_MODE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-HIERARCHY"><constant>DTV_HIERARCHY</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-LNA"><constant>DTV_LNA</constant></link></para></listitem>
			</itemizedlist>
			<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
		<section id="dvbt2-params">
			<title>DVB-T2 delivery system</title>
			<para>DVB-T2 support is currently in the early stages
			of development, so expect that this section maygrow and become
			more detailed with time.</para>
		<para>The following parameters are valid for DVB-T2:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CODE-RATE-HP"><constant>DTV_CODE_RATE_HP</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CODE-RATE-LP"><constant>DTV_CODE_RATE_LP</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-GUARD-INTERVAL"><constant>DTV_GUARD_INTERVAL</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TRANSMISSION-MODE"><constant>DTV_TRANSMISSION_MODE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-HIERARCHY"><constant>DTV_HIERARCHY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-STREAM-ID"><constant>DTV_STREAM_ID</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-LNA"><constant>DTV_LNA</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
		<section id="isdbt">
		<title>ISDB-T delivery system</title>
		<para>This ISDB-T/ISDB-Tsb API extension should reflect all information
			needed to tune any ISDB-T/ISDB-Tsb hardware. Of course it is possible
			that some very sophisticated devices won't need certain parameters to
			tune.</para>
		<para>The information given here should help application writers to know how
			to handle ISDB-T and ISDB-Tsb hardware using the Linux DVB-API.</para>
		<para>The details given here about ISDB-T and ISDB-Tsb are just enough to
			basically show the dependencies between the needed parameter values,
			but surely some information is left out. For more detailed information
			see the following documents:</para>
		<para>ARIB STD-B31 - "Transmission System for Digital Terrestrial
			Television Broadcasting" and</para>
		<para>ARIB TR-B14 - "Operational Guidelines for Digital Terrestrial
			Television Broadcasting".</para>
		<para>In order to understand the ISDB specific parameters,
			one has to have some knowledge the channel structure in
			ISDB-T and ISDB-Tsb. I.e. it has to be known to
			the reader that an ISDB-T channel consists of 13 segments,
			that it can have up to 3 layer sharing those segments,
			and things like that.</para>
		<para>The following parameters are valid for ISDB-T:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-GUARD-INTERVAL"><constant>DTV_GUARD_INTERVAL</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TRANSMISSION-MODE"><constant>DTV_TRANSMISSION_MODE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-ENABLED"><constant>DTV_ISDBT_LAYER_ENABLED</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-PARTIAL-RECEPTION"><constant>DTV_ISDBT_PARTIAL_RECEPTION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-SOUND-BROADCASTING"><constant>DTV_ISDBT_SOUND_BROADCASTING</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-SB-SUBCHANNEL-ID"><constant>DTV_ISDBT_SB_SUBCHANNEL_ID</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-SB-SEGMENT-IDX"><constant>DTV_ISDBT_SB_SEGMENT_IDX</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-SB-SEGMENT-COUNT"><constant>DTV_ISDBT_SB_SEGMENT_COUNT</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-FEC"><constant>DTV_ISDBT_LAYERA_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-MODULATION"><constant>DTV_ISDBT_LAYERA_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-SEGMENT-COUNT"><constant>DTV_ISDBT_LAYERA_SEGMENT_COUNT</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-TIME-INTERLEAVING"><constant>DTV_ISDBT_LAYERA_TIME_INTERLEAVING</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-FEC"><constant>DTV_ISDBT_LAYERB_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-MODULATION"><constant>DTV_ISDBT_LAYERB_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-SEGMENT-COUNT"><constant>DTV_ISDBT_LAYERB_SEGMENT_COUNT</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-TIME-INTERLEAVING"><constant>DTV_ISDBT_LAYERB_TIME_INTERLEAVING</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-FEC"><constant>DTV_ISDBT_LAYERC_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-MODULATION"><constant>DTV_ISDBT_LAYERC_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-SEGMENT-COUNT"><constant>DTV_ISDBT_LAYERC_SEGMENT_COUNT</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ISDBT-LAYER-TIME-INTERLEAVING"><constant>DTV_ISDBT_LAYERC_TIME_INTERLEAVING</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
		<section id="atsc-params">
			<title>ATSC delivery system</title>
			<para>The following parameters are valid for ATSC:</para>
			<itemizedlist mark='opencircle'>
				<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
			</itemizedlist>
			<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
		<section id="atscmh-params">
			<title>ATSC-MH delivery system</title>
			<para>The following parameters are valid for ATSC-MH:</para>
			<itemizedlist mark='opencircle'>
				<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-FIC-VER"><constant>DTV_ATSCMH_FIC_VER</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-PARADE-ID"><constant>DTV_ATSCMH_PARADE_ID</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-NOG"><constant>DTV_ATSCMH_NOG</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-TNOG"><constant>DTV_ATSCMH_TNOG</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SGN"><constant>DTV_ATSCMH_SGN</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-PRC"><constant>DTV_ATSCMH_PRC</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-RS-FRAME-MODE"><constant>DTV_ATSCMH_RS_FRAME_MODE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-RS-FRAME-ENSEMBLE"><constant>DTV_ATSCMH_RS_FRAME_ENSEMBLE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-RS-CODE-MODE-PRI"><constant>DTV_ATSCMH_RS_CODE_MODE_PRI</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-RS-CODE-MODE-SEC"><constant>DTV_ATSCMH_RS_CODE_MODE_SEC</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SCCC-BLOCK-MODE"><constant>DTV_ATSCMH_SCCC_BLOCK_MODE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SCCC-CODE-MODE-A"><constant>DTV_ATSCMH_SCCC_CODE_MODE_A</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SCCC-CODE-MODE-B"><constant>DTV_ATSCMH_SCCC_CODE_MODE_B</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SCCC-CODE-MODE-C"><constant>DTV_ATSCMH_SCCC_CODE_MODE_C</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-ATSCMH-SCCC-CODE-MODE-D"><constant>DTV_ATSCMH_SCCC_CODE_MODE_D</constant></link></para></listitem>
			</itemizedlist>
			<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
		<section id="dtmb-params">
			<title>DTMB delivery system</title>
			<para>The following parameters are valid for DTMB:</para>
			<itemizedlist mark='opencircle'>
				<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-BANDWIDTH-HZ"><constant>DTV_BANDWIDTH_HZ</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-INNER-FEC"><constant>DTV_INNER_FEC</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-GUARD-INTERVAL"><constant>DTV_GUARD_INTERVAL</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-TRANSMISSION-MODE"><constant>DTV_TRANSMISSION_MODE</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-INTERLEAVING"><constant>DTV_INTERLEAVING</constant></link></para></listitem>
				<listitem><para><link linkend="DTV-LNA"><constant>DTV_LNA</constant></link></para></listitem>
			</itemizedlist>
			<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		</section>
	</section>
	<section id="frontend-property-cable-systems">
	<title>Properties used on cable delivery systems</title>
	<section id="dvbc-params">
		<title>DVB-C delivery system</title>
		<para>The DVB-C Annex-A is the widely used cable standard. Transmission uses QAM modulation.</para>
		<para>The DVB-C Annex-C is optimized for 6MHz, and is used in Japan. It supports a subset of the Annex A modulation types, and a roll-off of 0.13, instead of 0.15</para>
		<para>The following parameters are valid for DVB-C Annex A/C:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-SYMBOL-RATE"><constant>DTV_SYMBOL_RATE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INNER-FEC"><constant>DTV_INNER_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-LNA"><constant>DTV_LNA</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
	</section>
	<section id="dvbc-annex-b-params">
		<title>DVB-C Annex B delivery system</title>
		<para>The DVB-C Annex-B is only used on a few Countries like the United States.</para>
		<para>The following parameters are valid for DVB-C Annex B:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-LNA"><constant>DTV_LNA</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
	</section>
	</section>
	<section id="frontend-property-satellite-systems">
	<title>Properties used on satellite delivery systems</title>
	<section id="dvbs-params">
		<title>DVB-S delivery system</title>
		<para>The following parameters are valid for DVB-S:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-SYMBOL-RATE"><constant>DTV_SYMBOL_RATE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INNER-FEC"><constant>DTV_INNER_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-VOLTAGE"><constant>DTV_VOLTAGE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TONE"><constant>DTV_TONE</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
		<para>Future implementations might add those two missing parameters:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-DISEQC-MASTER"><constant>DTV_DISEQC_MASTER</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DISEQC-SLAVE-REPLY"><constant>DTV_DISEQC_SLAVE_REPLY</constant></link></para></listitem>
		</itemizedlist>
	</section>
	<section id="dvbs2-params">
		<title>DVB-S2 delivery system</title>
		<para>In addition to all parameters valid for DVB-S, DVB-S2 supports the following parameters:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-PILOT"><constant>DTV_PILOT</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-ROLLOFF"><constant>DTV_ROLLOFF</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-STREAM-ID"><constant>DTV_STREAM_ID</constant></link></para></listitem>
		</itemizedlist>
		<para>In addition, the <link linkend="frontend-stat-properties">DTV QoS statistics</link> are also valid.</para>
	</section>
	<section id="turbo-params">
		<title>Turbo code delivery system</title>
		<para>In addition to all parameters valid for DVB-S, turbo code supports the following parameters:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-MODULATION"><constant>DTV_MODULATION</constant></link></para></listitem>
		</itemizedlist>
	</section>
	<section id="isdbs-params">
		<title>ISDB-S delivery system</title>
		<para>The following parameters are valid for ISDB-S:</para>
		<itemizedlist mark='opencircle'>
			<listitem><para><link linkend="DTV-API-VERSION"><constant>DTV_API_VERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-DELIVERY-SYSTEM"><constant>DTV_DELIVERY_SYSTEM</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-TUNE"><constant>DTV_TUNE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-CLEAR"><constant>DTV_CLEAR</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-FREQUENCY"><constant>DTV_FREQUENCY</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INVERSION"><constant>DTV_INVERSION</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-SYMBOL-RATE"><constant>DTV_SYMBOL_RATE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-INNER-FEC"><constant>DTV_INNER_FEC</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-VOLTAGE"><constant>DTV_VOLTAGE</constant></link></para></listitem>
			<listitem><para><link linkend="DTV-STREAM-ID"><constant>DTV_STREAM_ID</constant></link></para></listitem>
		</itemizedlist>
	</section>
	</section>
</section>
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       <title>Changes</title>

  <para>The following chapters document the evolution of the V4L2 API,
errata or extensions. They are also intended to help application and
driver writers to port or update their code.</para>

  <section id="diff-v4l">
    <title>Differences between V4L and V4L2</title>

    <para>The Video For Linux API was first introduced in Linux 2.1 to
unify and replace various TV and radio device related interfaces,
developed independently by driver writers in prior years. Starting
with Linux 2.5 the much improved V4L2 API replaces the V4L API.
The support for the old V4L calls were removed from Kernel, but the
library <xref linkend="libv4l" /> supports the conversion of a V4L
API system call into a V4L2 one.</para>

    <section>
      <title>Opening and Closing Devices</title>

      <para>For compatibility reasons the character device file names
recommended for V4L2 video capture, overlay, radio and raw
vbi capture devices did not change from those used by V4L. They are
listed in <xref linkend="devices" /> and below in <xref
	  linkend="v4l-dev" />.</para>

      <para>The teletext devices (minor range 192-223) have been removed in
V4L2 and no longer exist. There is no hardware available anymore for handling
pure teletext. Instead raw or sliced VBI is used.</para>

      <para>The V4L <filename>videodev</filename> module automatically
assigns minor numbers to drivers in load order, depending on the
registered device type. We recommend that V4L2 drivers by default
register devices with the same numbers, but the system administrator
can assign arbitrary minor numbers using driver module options. The
major device number remains 81.</para>

      <table id="v4l-dev">
	<title>V4L Device Types, Names and Numbers</title>
	<tgroup cols="3">
	  <thead>
	    <row>
	      <entry>Device Type</entry>
	      <entry>File Name</entry>
	      <entry>Minor Numbers</entry>
	    </row>
	  </thead>
	  <tbody valign="top">
	    <row>
	      <entry>Video capture and overlay</entry>
	      <entry><para><filename>/dev/video</filename> and
<filename>/dev/bttv0</filename><footnote> <para>According to
Documentation/devices.txt these should be symbolic links to
<filename>/dev/video0</filename>. Note the original bttv interface is
not compatible with V4L or V4L2.</para> </footnote>,
<filename>/dev/video0</filename> to
<filename>/dev/video63</filename></para></entry>
	      <entry>0-63</entry>
	    </row>
	    <row>
	      <entry>Radio receiver</entry>
	      <entry><para><filename>/dev/radio</filename><footnote>
		    <para>According to
<filename>Documentation/devices.txt</filename> a symbolic link to
<filename>/dev/radio0</filename>.</para>
		  </footnote>, <filename>/dev/radio0</filename> to
<filename>/dev/radio63</filename></para></entry>
	      <entry>64-127</entry>
	    </row>
	    <row>
	      <entry>Raw VBI capture</entry>
	      <entry><para><filename>/dev/vbi</filename>,
<filename>/dev/vbi0</filename> to
<filename>/dev/vbi31</filename></para></entry>
	      <entry>224-255</entry>
	    </row>
	  </tbody>
	</tgroup>
      </table>

      <para>V4L prohibits (or used to prohibit) multiple opens of a
device file. V4L2 drivers <emphasis>may</emphasis> support multiple
opens, see <xref linkend="open" /> for details and consequences.</para>

      <para>V4L drivers respond to V4L2 ioctls with an &EINVAL;.</para>
    </section>

    <section>
      <title>Querying Capabilities</title>

      <para>The V4L <constant>VIDIOCGCAP</constant> ioctl is
equivalent to V4L2's &VIDIOC-QUERYCAP;.</para>

      <para>The <structfield>name</structfield> field in struct
<structname>video_capability</structname> became
<structfield>card</structfield> in &v4l2-capability;,
<structfield>type</structfield> was replaced by
<structfield>capabilities</structfield>. Note V4L2 does not
distinguish between device types like this, better think of basic
video input, video output and radio devices supporting a set of
related functions like video capturing, video overlay and VBI
capturing. See <xref linkend="open" /> for an
introduction.<informaltable>
	  <tgroup cols="3">
	    <thead>
	      <row>
		<entry>struct
<structname>video_capability</structname>
<structfield>type</structfield></entry>
		<entry>&v4l2-capability;
<structfield>capabilities</structfield> flags</entry>
		<entry>Purpose</entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry><constant>VID_TYPE_CAPTURE</constant></entry>
		<entry><constant>V4L2_CAP_VIDEO_CAPTURE</constant></entry>
		<entry>The <link linkend="capture">video
capture</link> interface is supported.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_TUNER</constant></entry>
		<entry><constant>V4L2_CAP_TUNER</constant></entry>
		<entry>The device has a <link linkend="tuner">tuner or
modulator</link>.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_TELETEXT</constant></entry>
		<entry><constant>V4L2_CAP_VBI_CAPTURE</constant></entry>
		<entry>The <link linkend="raw-vbi">raw VBI
capture</link> interface is supported.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_OVERLAY</constant></entry>
		<entry><constant>V4L2_CAP_VIDEO_OVERLAY</constant></entry>
		<entry>The <link linkend="overlay">video
overlay</link> interface is supported.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_CHROMAKEY</constant></entry>
		<entry><constant>V4L2_FBUF_CAP_CHROMAKEY</constant> in
field <structfield>capability</structfield> of
&v4l2-framebuffer;</entry>
		<entry>Whether chromakey overlay is supported. For
more information on overlay see
<xref linkend="overlay" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_CLIPPING</constant></entry>
		<entry><constant>V4L2_FBUF_CAP_LIST_CLIPPING</constant>
and <constant>V4L2_FBUF_CAP_BITMAP_CLIPPING</constant> in field
<structfield>capability</structfield> of &v4l2-framebuffer;</entry>
		<entry>Whether clipping the overlaid image is
supported, see <xref linkend="overlay" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_FRAMERAM</constant></entry>
		<entry><constant>V4L2_FBUF_CAP_EXTERNOVERLAY</constant>
<emphasis>not set</emphasis> in field
<structfield>capability</structfield> of &v4l2-framebuffer;</entry>
		<entry>Whether overlay overwrites frame buffer memory,
see <xref linkend="overlay" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_SCALES</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>This flag indicates if the hardware can scale
images. The V4L2 API implies the scale factor by setting the cropping
dimensions and image size with the &VIDIOC-S-CROP; and &VIDIOC-S-FMT;
ioctl, respectively. The driver returns the closest sizes possible.
For more information on cropping and scaling see <xref
		    linkend="crop" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_MONOCHROME</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>Applications can enumerate the supported image
formats with the &VIDIOC-ENUM-FMT; ioctl to determine if the device
supports grey scale capturing only. For more information on image
formats see <xref linkend="pixfmt" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_SUBCAPTURE</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>Applications can call the &VIDIOC-G-CROP; ioctl
to determine if the device supports capturing a subsection of the full
picture ("cropping" in V4L2). If not, the ioctl returns the &EINVAL;.
For more information on cropping and scaling see <xref
		    linkend="crop" />.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_MPEG_DECODER</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>Applications can enumerate the supported image
formats with the &VIDIOC-ENUM-FMT; ioctl to determine if the device
supports MPEG streams.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_MPEG_ENCODER</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>See above.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_MJPEG_DECODER</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>See above.</entry>
	      </row>
	      <row>
		<entry><constant>VID_TYPE_MJPEG_ENCODER</constant></entry>
		<entry><constant>-</constant></entry>
		<entry>See above.</entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable></para>

      <para>The <structfield>audios</structfield> field was replaced
by <structfield>capabilities</structfield> flag
<constant>V4L2_CAP_AUDIO</constant>, indicating
<emphasis>if</emphasis> the device has any audio inputs or outputs. To
determine their number applications can enumerate audio inputs with
the &VIDIOC-G-AUDIO; ioctl. The audio ioctls are described in <xref
	  linkend="audio" />.</para>

      <para>The <structfield>maxwidth</structfield>,
<structfield>maxheight</structfield>,
<structfield>minwidth</structfield> and
<structfield>minheight</structfield> fields were removed. Calling the
&VIDIOC-S-FMT; or &VIDIOC-TRY-FMT; ioctl with the desired dimensions
returns the closest size possible, taking into account the current
video standard, cropping and scaling limitations.</para>
    </section>

    <section>
      <title>Video Sources</title>

      <para>V4L provides the <constant>VIDIOCGCHAN</constant> and
<constant>VIDIOCSCHAN</constant> ioctl using struct
<structname>video_channel</structname> to enumerate
the video inputs of a V4L device. The equivalent V4L2 ioctls
are &VIDIOC-ENUMINPUT;, &VIDIOC-G-INPUT; and &VIDIOC-S-INPUT;
using &v4l2-input; as discussed in <xref linkend="video" />.</para>

      <para>The <structfield>channel</structfield> field counting
inputs was renamed to <structfield>index</structfield>, the video
input types were renamed as follows: <informaltable>
	  <tgroup cols="2">
	    <thead>
	      <row>
		<entry>struct <structname>video_channel</structname>
<structfield>type</structfield></entry>
		<entry>&v4l2-input;
<structfield>type</structfield></entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry><constant>VIDEO_TYPE_TV</constant></entry>
		<entry><constant>V4L2_INPUT_TYPE_TUNER</constant></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_TYPE_CAMERA</constant></entry>
		<entry><constant>V4L2_INPUT_TYPE_CAMERA</constant></entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable></para>

      <para>Unlike the <structfield>tuners</structfield> field
expressing the number of tuners of this input, V4L2 assumes each video
input is connected to at most one tuner. However a tuner can have more
than one input, &ie; RF connectors, and a device can have multiple
tuners. The index number of the tuner associated with the input, if
any, is stored in field <structfield>tuner</structfield> of
&v4l2-input;. Enumeration of tuners is discussed in <xref
	  linkend="tuner" />.</para>

      <para>The redundant <constant>VIDEO_VC_TUNER</constant> flag was
dropped. Video inputs associated with a tuner are of type
<constant>V4L2_INPUT_TYPE_TUNER</constant>. The
<constant>VIDEO_VC_AUDIO</constant> flag was replaced by the
<structfield>audioset</structfield> field. V4L2 considers devices with
up to 32 audio inputs. Each set bit in the
<structfield>audioset</structfield> field represents one audio input
this video input combines with. For information about audio inputs and
how to switch between them see <xref linkend="audio" />.</para>

      <para>The <structfield>norm</structfield> field describing the
supported video standards was replaced by
<structfield>std</structfield>. The V4L specification mentions a flag
<constant>VIDEO_VC_NORM</constant> indicating whether the standard can
be changed. This flag was a later addition together with the
<structfield>norm</structfield> field and has been removed in the
meantime. V4L2 has a similar, albeit more comprehensive approach
to video standards, see <xref linkend="standard" /> for more
information.</para>
    </section>

    <section>
      <title>Tuning</title>

      <para>The V4L <constant>VIDIOCGTUNER</constant> and
<constant>VIDIOCSTUNER</constant> ioctl and struct
<structname>video_tuner</structname> can be used to enumerate the
tuners of a V4L TV or radio device. The equivalent V4L2 ioctls are
&VIDIOC-G-TUNER; and &VIDIOC-S-TUNER; using &v4l2-tuner;. Tuners are
covered in <xref linkend="tuner" />.</para>

      <para>The <structfield>tuner</structfield> field counting tuners
was renamed to <structfield>index</structfield>. The fields
<structfield>name</structfield>, <structfield>rangelow</structfield>
and <structfield>rangehigh</structfield> remained unchanged.</para>

      <para>The <constant>VIDEO_TUNER_PAL</constant>,
<constant>VIDEO_TUNER_NTSC</constant> and
<constant>VIDEO_TUNER_SECAM</constant> flags indicating the supported
video standards were dropped. This information is now contained in the
associated &v4l2-input;. No replacement exists for the
<constant>VIDEO_TUNER_NORM</constant> flag indicating whether the
video standard can be switched. The <structfield>mode</structfield>
field to select a different video standard was replaced by a whole new
set of ioctls and structures described in <xref linkend="standard" />.
Due to its ubiquity it should be mentioned the BTTV driver supports
several standards in addition to the regular
<constant>VIDEO_MODE_PAL</constant> (0),
<constant>VIDEO_MODE_NTSC</constant>,
<constant>VIDEO_MODE_SECAM</constant> and
<constant>VIDEO_MODE_AUTO</constant> (3). Namely N/PAL Argentina,
M/PAL, N/PAL, and NTSC Japan with numbers 3-6 (sic).</para>

      <para>The <constant>VIDEO_TUNER_STEREO_ON</constant> flag
indicating stereo reception became
<constant>V4L2_TUNER_SUB_STEREO</constant> in field
<structfield>rxsubchans</structfield>. This field also permits the
detection of monaural and bilingual audio, see the definition of
&v4l2-tuner; for details. Presently no replacement exists for the
<constant>VIDEO_TUNER_RDS_ON</constant> and
<constant>VIDEO_TUNER_MBS_ON</constant> flags.</para>

      <para> The <constant>VIDEO_TUNER_LOW</constant> flag was renamed
to <constant>V4L2_TUNER_CAP_LOW</constant> in the &v4l2-tuner;
<structfield>capability</structfield> field.</para>

      <para>The <constant>VIDIOCGFREQ</constant> and
<constant>VIDIOCSFREQ</constant> ioctl to change the tuner frequency
where renamed to &VIDIOC-G-FREQUENCY; and  &VIDIOC-S-FREQUENCY;. They
take a pointer to a &v4l2-frequency; instead of an unsigned long
integer.</para>
    </section>

    <section id="v4l-image-properties">
      <title>Image Properties</title>

      <para>V4L2 has no equivalent of the
<constant>VIDIOCGPICT</constant> and <constant>VIDIOCSPICT</constant>
ioctl and struct <structname>video_picture</structname>. The following
fields where replaced by V4L2 controls accessible with the
&VIDIOC-QUERYCTRL;, &VIDIOC-G-CTRL; and &VIDIOC-S-CTRL; ioctls:<informaltable>
	  <tgroup cols="2">
	    <thead>
	      <row>
		<entry>struct <structname>video_picture</structname></entry>
		<entry>V4L2 Control ID</entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry><structfield>brightness</structfield></entry>
		<entry><constant>V4L2_CID_BRIGHTNESS</constant></entry>
	      </row>
	      <row>
		<entry><structfield>hue</structfield></entry>
		<entry><constant>V4L2_CID_HUE</constant></entry>
	      </row>
	      <row>
		<entry><structfield>colour</structfield></entry>
		<entry><constant>V4L2_CID_SATURATION</constant></entry>
	      </row>
	      <row>
		<entry><structfield>contrast</structfield></entry>
		<entry><constant>V4L2_CID_CONTRAST</constant></entry>
	      </row>
	      <row>
		<entry><structfield>whiteness</structfield></entry>
		<entry><constant>V4L2_CID_WHITENESS</constant></entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable></para>

      <para>The V4L picture controls are assumed to range from 0 to
65535 with no particular reset value. The V4L2 API permits arbitrary
limits and defaults which can be queried with the &VIDIOC-QUERYCTRL;
ioctl. For general information about controls see <xref
linkend="control" />.</para>

      <para>The <structfield>depth</structfield> (average number of
bits per pixel) of a video image is implied by the selected image
format. V4L2 does not explicitly provide such information assuming
applications recognizing the format are aware of the image depth and
others need not know. The <structfield>palette</structfield> field
moved into the &v4l2-pix-format;:<informaltable>
	  <tgroup cols="2">
	    <thead>
	      <row>
		<entry>struct <structname>video_picture</structname>
<structfield>palette</structfield></entry>
		<entry>&v4l2-pix-format;
<structfield>pixfmt</structfield></entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry><constant>VIDEO_PALETTE_GREY</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-GREY"><constant>V4L2_PIX_FMT_GREY</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_HI240</constant></entry>
		<entry><para><link
linkend="pixfmt-reserved"><constant>V4L2_PIX_FMT_HI240</constant></link><footnote>
		      <para>This is a custom format used by the BTTV
driver, not one of the V4L2 standard formats.</para>
		    </footnote></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_RGB565</constant></entry>
		<entry><para><link
linkend="pixfmt-rgb"><constant>V4L2_PIX_FMT_RGB565</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_RGB555</constant></entry>
		<entry><para><link
linkend="pixfmt-rgb"><constant>V4L2_PIX_FMT_RGB555</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_RGB24</constant></entry>
		<entry><para><link
linkend="pixfmt-rgb"><constant>V4L2_PIX_FMT_BGR24</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_RGB32</constant></entry>
		<entry><para><link
linkend="pixfmt-rgb"><constant>V4L2_PIX_FMT_BGR32</constant></link><footnote>
		      <para>Presumably all V4L RGB formats are
little-endian, although some drivers might interpret them according to machine endianness. V4L2 defines little-endian, big-endian and red/blue
swapped variants. For details see <xref linkend="pixfmt-rgb" />.</para>
		    </footnote></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV422</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YUYV"><constant>V4L2_PIX_FMT_YUYV</constant></link></para></entry>
	      </row>
	      <row>
		<entry><para><constant>VIDEO_PALETTE_YUYV</constant><footnote>
		      <para><constant>VIDEO_PALETTE_YUV422</constant>
and <constant>VIDEO_PALETTE_YUYV</constant> are the same formats. Some
V4L drivers respond to one, some to the other.</para>
		    </footnote></para></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YUYV"><constant>V4L2_PIX_FMT_YUYV</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_UYVY</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-UYVY"><constant>V4L2_PIX_FMT_UYVY</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV420</constant></entry>
		<entry>None</entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV411</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-Y41P"><constant>V4L2_PIX_FMT_Y41P</constant></link><footnote>
		      <para>Not to be confused with
<constant>V4L2_PIX_FMT_YUV411P</constant>, which is a planar
format.</para> </footnote></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_RAW</constant></entry>
		<entry><para>None<footnote> <para>V4L explains this
as: "RAW capture (BT848)"</para> </footnote></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV422P</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YUV422P"><constant>V4L2_PIX_FMT_YUV422P</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV411P</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YUV411P"><constant>V4L2_PIX_FMT_YUV411P</constant></link><footnote>
		      <para>Not to be confused with
<constant>V4L2_PIX_FMT_Y41P</constant>, which is a packed
format.</para> </footnote></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV420P</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YVU420"><constant>V4L2_PIX_FMT_YVU420</constant></link></para></entry>
	      </row>
	      <row>
		<entry><constant>VIDEO_PALETTE_YUV410P</constant></entry>
		<entry><para><link
linkend="V4L2-PIX-FMT-YVU410"><constant>V4L2_PIX_FMT_YVU410</constant></link></para></entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable></para>

      <para>V4L2 image formats are defined in <xref
linkend="pixfmt" />. The image format can be selected with the
&VIDIOC-S-FMT; ioctl.</para>
    </section>

    <section>
      <title>Audio</title>

      <para>The <constant>VIDIOCGAUDIO</constant> and
<constant>VIDIOCSAUDIO</constant> ioctl and struct
<structname>video_audio</structname> are used to enumerate the
audio inputs of a V4L device. The equivalent V4L2 ioctls are
&VIDIOC-G-AUDIO; and &VIDIOC-S-AUDIO; using &v4l2-audio; as
discussed in <xref linkend="audio" />.</para>

      <para>The <structfield>audio</structfield> "channel number"
field counting audio inputs was renamed to
<structfield>index</structfield>.</para>

      <para>On <constant>VIDIOCSAUDIO</constant> the
<structfield>mode</structfield> field selects <emphasis>one</emphasis>
of the <constant>VIDEO_SOUND_MONO</constant>,
<constant>VIDEO_SOUND_STEREO</constant>,
<constant>VIDEO_SOUND_LANG1</constant> or
<constant>VIDEO_SOUND_LANG2</constant> audio demodulation modes. When
the current audio standard is BTSC
<constant>VIDEO_SOUND_LANG2</constant> refers to SAP and
<constant>VIDEO_SOUND_LANG1</constant> is meaningless. Also
undocumented in the V4L specification, there is no way to query the
selected mode. On <constant>VIDIOCGAUDIO</constant> the driver returns
the <emphasis>actually received</emphasis> audio programmes in this
field. In the V4L2 API this information is stored in the &v4l2-tuner;
<structfield>rxsubchans</structfield> and
<structfield>audmode</structfield> fields, respectively. See <xref
linkend="tuner" /> for more information on tuners. Related to audio
modes &v4l2-audio; also reports if this is a mono or stereo
input, regardless if the source is a tuner.</para>

      <para>The following fields where replaced by V4L2 controls
accessible with the &VIDIOC-QUERYCTRL;, &VIDIOC-G-CTRL; and
&VIDIOC-S-CTRL; ioctls:<informaltable>
	  <tgroup cols="2">
	    <thead>
	      <row>
		<entry>struct
<structname>video_audio</structname></entry>
		<entry>V4L2 Control ID</entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry><structfield>volume</structfield></entry>
		<entry><constant>V4L2_CID_AUDIO_VOLUME</constant></entry>
	      </row>
	      <row>
		<entry><structfield>bass</structfield></entry>
		<entry><constant>V4L2_CID_AUDIO_BASS</constant></entry>
	      </row>
	      <row>
		<entry><structfield>treble</structfield></entry>
		<entry><constant>V4L2_CID_AUDIO_TREBLE</constant></entry>
	      </row>
	      <row>
		<entry><structfield>balance</structfield></entry>
		<entry><constant>V4L2_CID_AUDIO_BALANCE</constant></entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable></para>

      <para>To determine which of these controls are supported by a
driver V4L provides the <structfield>flags</structfield>
<constant>VIDEO_AUDIO_VOLUME</constant>,
<constant>VIDEO_AUDIO_BASS</constant>,
<constant>VIDEO_AUDIO_TREBLE</constant> and
<constant>VIDEO_AUDIO_BALANCE</constant>. In the V4L2 API the
&VIDIOC-QUERYCTRL; ioctl reports if the respective control is
supported. Accordingly the <constant>VIDEO_AUDIO_MUTABLE</constant>
and <constant>VIDEO_AUDIO_MUTE</constant> flags where replaced by the
boolean <constant>V4L2_CID_AUDIO_MUTE</constant> control.</para>

      <para>All V4L2 controls have a <structfield>step</structfield>
attribute replacing the struct <structname>video_audio</structname>
<structfield>step</structfield> field. The V4L audio controls are
assumed to range from 0 to 65535 with no particular reset value. The
V4L2 API permits arbitrary limits and defaults which can be queried
with the &VIDIOC-QUERYCTRL; ioctl. For general information about
controls see <xref linkend="control" />.</para>
    </section>

    <section>
      <title>Frame Buffer Overlay</title>

      <para>The V4L2 ioctls equivalent to
<constant>VIDIOCGFBUF</constant> and <constant>VIDIOCSFBUF</constant>
are &VIDIOC-G-FBUF; and &VIDIOC-S-FBUF;. The
<structfield>base</structfield> field of struct
<structname>video_buffer</structname> remained unchanged, except V4L2
defines a flag to indicate non-destructive overlays instead of a
<constant>NULL</constant> pointer. All other fields moved into the
&v4l2-pix-format; <structfield>fmt</structfield> substructure of
&v4l2-framebuffer;. The <structfield>depth</structfield> field was
replaced by <structfield>pixelformat</structfield>. See <xref
	  linkend="pixfmt-rgb" /> for a list of RGB formats and their
respective color depths.</para>

      <para>Instead of the special ioctls
<constant>VIDIOCGWIN</constant> and <constant>VIDIOCSWIN</constant>
V4L2 uses the general-purpose data format negotiation ioctls
&VIDIOC-G-FMT; and &VIDIOC-S-FMT;. They take a pointer to a
&v4l2-format; as argument. Here the <structfield>win</structfield>
member of the <structfield>fmt</structfield> union is used, a
&v4l2-window;.</para>

      <para>The <structfield>x</structfield>,
<structfield>y</structfield>, <structfield>width</structfield> and
<structfield>height</structfield> fields of struct
<structname>video_window</structname> moved into &v4l2-rect;
substructure <structfield>w</structfield> of struct
<structname>v4l2_window</structname>. The
<structfield>chromakey</structfield>,
<structfield>clips</structfield>, and
<structfield>clipcount</structfield> fields remained unchanged. Struct
<structname>video_clip</structname> was renamed to &v4l2-clip;, also
containing a struct <structname>v4l2_rect</structname>, but the
semantics are still the same.</para>

      <para>The <constant>VIDEO_WINDOW_INTERLACE</constant> flag was
dropped. Instead applications must set the
<structfield>field</structfield> field to
<constant>V4L2_FIELD_ANY</constant> or
<constant>V4L2_FIELD_INTERLACED</constant>. The
<constant>VIDEO_WINDOW_CHROMAKEY</constant> flag moved into
&v4l2-framebuffer;, under the new name
<constant>V4L2_FBUF_FLAG_CHROMAKEY</constant>.</para>

      <para>In V4L, storing a bitmap pointer in
<structfield>clips</structfield> and setting
<structfield>clipcount</structfield> to
<constant>VIDEO_CLIP_BITMAP</constant> (-1) requests bitmap
clipping, using a fixed size bitmap of 1024 &times; 625 bits. Struct
<structname>v4l2_window</structname> has a separate
<structfield>bitmap</structfield> pointer field for this purpose and
the bitmap size is determined by <structfield>w.width</structfield> and
<structfield>w.height</structfield>.</para>

      <para>The <constant>VIDIOCCAPTURE</constant> ioctl to enable or
disable overlay was renamed to &VIDIOC-OVERLAY;.</para>
    </section>

    <section>
      <title>Cropping</title>

      <para>To capture only a subsection of the full picture V4L
defines the <constant>VIDIOCGCAPTURE</constant> and
<constant>VIDIOCSCAPTURE</constant> ioctls using struct
<structname>video_capture</structname>. The equivalent V4L2 ioctls are
&VIDIOC-G-CROP; and &VIDIOC-S-CROP; using &v4l2-crop;, and the related
&VIDIOC-CROPCAP; ioctl. This is a rather complex matter, see
<xref linkend="crop" /> for details.</para>

      <para>The <structfield>x</structfield>,
<structfield>y</structfield>, <structfield>width</structfield> and
<structfield>height</structfield> fields moved into &v4l2-rect;
substructure <structfield>c</structfield> of struct
<structname>v4l2_crop</structname>. The
<structfield>decimation</structfield> field was dropped. In the V4L2
API the scaling factor is implied by the size of the cropping
rectangle and the size of the captured or overlaid image.</para>

      <para>The <constant>VIDEO_CAPTURE_ODD</constant>
and <constant>VIDEO_CAPTURE_EVEN</constant> flags to capture only the
odd or even field, respectively, were replaced by
<constant>V4L2_FIELD_TOP</constant> and
<constant>V4L2_FIELD_BOTTOM</constant> in the field named
<structfield>field</structfield> of &v4l2-pix-format; and
&v4l2-window;. These structures are used to select a capture or
overlay format with the &VIDIOC-S-FMT; ioctl.</para>
    </section>

    <section>
      <title>Reading Images, Memory Mapping</title>

      <section>
	<title>Capturing using the read method</title>

	<para>There is no essential difference between reading images
from a V4L or V4L2 device using the &func-read; function, however V4L2
drivers are not required to support this I/O method. Applications can
determine if the function is available with the &VIDIOC-QUERYCAP;
ioctl. All V4L2 devices exchanging data with applications must support
the &func-select; and &func-poll; functions.</para>

	<para>To select an image format and size, V4L provides the
<constant>VIDIOCSPICT</constant> and <constant>VIDIOCSWIN</constant>
ioctls. V4L2 uses the general-purpose data format negotiation ioctls
&VIDIOC-G-FMT; and &VIDIOC-S-FMT;. They take a pointer to a
&v4l2-format; as argument, here the &v4l2-pix-format; named
<structfield>pix</structfield> of its <structfield>fmt</structfield>
union is used.</para>

	<para>For more information about the V4L2 read interface see
<xref linkend="rw" />.</para>
      </section>
      <section>
	<title>Capturing using memory mapping</title>

	<para>Applications can read from V4L devices by mapping
buffers in device memory, or more often just buffers allocated in
DMA-able system memory, into their address space. This avoids the data
copying overhead of the read method. V4L2 supports memory mapping as
well, with a few differences.</para>

	<informaltable>
	  <tgroup cols="2">
	    <thead>
	      <row>
		<entry>V4L</entry>
		<entry>V4L2</entry>
	      </row>
	    </thead>
	    <tbody valign="top">
	      <row>
		<entry></entry>
		<entry>The image format must be selected before
buffers are allocated, with the &VIDIOC-S-FMT; ioctl. When no format
is selected the driver may use the last, possibly by another
application requested format.</entry>
	      </row>
	      <row>
		<entry><para>Applications cannot change the number of
buffers. The it is built into the driver, unless it has a module
option to change the number when the driver module is
loaded.</para></entry>
		<entry><para>The &VIDIOC-REQBUFS; ioctl allocates the
desired number of buffers, this is a required step in the initialization
sequence.</para></entry>
	      </row>
	      <row>
		<entry><para>Drivers map all buffers as one contiguous
range of memory. The <constant>VIDIOCGMBUF</constant> ioctl is
available to query the number of buffers, the offset of each buffer
from the start of the virtual file, and the overall amount of memory
used, which can be used as arguments for the &func-mmap;
function.</para></entry>
		<entry><para>Buffers are individually mapped. The
offset and size of each buffer can be determined with the
&VIDIOC-QUERYBUF; ioctl.</para></entry>
	      </row>
	      <row>
		<entry><para>The <constant>VIDIOCMCAPTURE</constant>
ioctl prepares a buffer for capturing. It also determines the image
format for this buffer. The ioctl returns immediately, eventually with
an &EAGAIN; if no video signal had been detected. When the driver
supports more than one buffer applications can call the ioctl multiple
times and thus have multiple outstanding capture
requests.</para><para>The <constant>VIDIOCSYNC</constant> ioctl
suspends execution until a particular buffer has been
filled.</para></entry>
		<entry><para>Drivers maintain an incoming and outgoing
queue. &VIDIOC-QBUF; enqueues any empty buffer into the incoming
queue. Filled buffers are dequeued from the outgoing queue with the
&VIDIOC-DQBUF; ioctl. To wait until filled buffers become available this
function, &func-select; or &func-poll; can be used. The
&VIDIOC-STREAMON; ioctl must be called once after enqueuing one or
more buffers to start capturing. Its counterpart
&VIDIOC-STREAMOFF; stops capturing and dequeues all buffers from both
queues. Applications can query the signal status, if known, with the
&VIDIOC-ENUMINPUT; ioctl.</para></entry>
	      </row>
	    </tbody>
	  </tgroup>
	</informaltable>

	<para>For a more in-depth discussion of memory mapping and
examples, see <xref linkend="mmap" />.</para>
      </section>
    </section>

    <section>
      <title>Reading Raw VBI Data</title>

      <para>Originally the V4L API did not specify a raw VBI capture
interface, only the device file <filename>/dev/vbi</filename> was
reserved for this purpose. The only driver supporting this interface
was the BTTV driver, de-facto defining the V4L VBI interface. Reading
from the device yields a raw VBI image with the following
parameters:<informaltable>
	    <tgroup cols="2">
	      <thead>
		<row>
		  <entry>&v4l2-vbi-format;</entry>
		  <