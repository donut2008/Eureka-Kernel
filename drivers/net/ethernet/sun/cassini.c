s-properties"/>
                for more information about properties.
              </para>
            </listitem>
            <listitem>
              <synopsis>void (*gamma_set)(struct drm_crtc *crtc, u16 *r, u16 *g, u16 *b,
                        uint32_t start, uint32_t size);</synopsis>
              <para>
                Apply a gamma table to the device. The operation is optional.
              </para>
            </listitem>
            <listitem>
              <synopsis>void (*destroy)(struct drm_crtc *crtc);</synopsis>
              <para>
                Destroy the CRTC when not needed anymore. See
                <xref linkend="drm-kms-init"/>.
              </para>
            </listitem>
          </itemizedlist>
        </sect4>
      </sect3>
    </sect2>
    <sect2>
      <title>Planes (struct <structname>drm_plane</structname>)</title>
      <para>
        A plane represents an image source that can be blended with or overlayed
	on top of a CRTC during the scanout process. Planes are associated with
	a frame buffer to crop a portion of the image memory (source) and
	optionally scale it to a destination size. The result is then blended
	with or overlayed on top of a CRTC.
      </para>
      <para>
      The DRM core recognizes three types of planes:
      <itemizedlist>
        <listitem>
        DRM_PLANE_TYPE_PRIMARY represents a "main" plane for a CRTC.  Primary
        planes are the planes operated upon by CRTC modesetting and flipping
        operations described in <xref linkend="drm-kms-crtcops"/>.
        </listitem>
        <listitem>
        DRM_PLANE_TYPE_CURSOR represents a "cursor" plane for a CRTC.  Cursor
        planes are the planes operated upon by the DRM_IOCTL_MODE_CURSOR and
        DRM_IOCTL_MODE_CURSOR2 ioctls.
        </listitem>
        <listitem>
        DRM_PLANE_TYPE_OVERLAY represents all non-primary, non-cursor planes.
        Some drivers refer to these types of planes as "sprites" internally.
        </listitem>
      </itemizedlist>
      For compatibility with legacy userspace, only overlay planes are made
      available to userspace by default.  Userspace clients may set the
      DRM_CLIENT_CAP_UNIVERSAL_PLANES client capability bit to indicate that
      they wish to receive a universal plane list containing all plane types.
      </para>
      <sect3>
        <title>Plane Initialization</title>
        <para>
          To create a plane, a KMS drivers allocates and
          zeroes an instances of struct <structname>drm_plane</structname>
          (possibly as part of a larger structure) and registers it with a call
          to <function>drm_universal_plane_init</function>. The function takes a bitmask
          of the CRTCs that can be associated with the plane, a pointer to the
          plane functions, a list of format supported formats, and the type of
          plane (primary, cursor, or overlay) being initialized.
        </para>
        <para>
          Cursor and overlay planes are optional.  All drivers should provide
          one primary plane per CRTC (although this requirement may change in
          the future); drivers that do not wish to provide special handling for
          primary planes may make use of the helper functions described in
          <xref linkend="drm-kms-planehelpers"/> to create and register a
          primary plane with standard capabilities.
        </para>
      </sect3>
      <sect3>
        <title>Plane Operations</title>
        <itemizedlist>
          <listitem>
            <synopsis>int (*update_plane)(struct drm_plane *plane, struct drm_crtc *crtc,
                        struct drm_framebuffer *fb, int crtc_x, int crtc_y,
                        unsigned int crtc_w, unsigned int crtc_h,
                        uint32_t src_x, uint32_t src_y,
                        uint32_t src_w, uint32_t src_h);</synopsis>
            <para>
              Enable and configure the plane to use the given CRTC and frame buffer.
            </para>
            <para>
              The source rectangle in frame buffer memory coordinates is given by
              the <parameter>src_x</parameter>, <parameter>src_y</parameter>,
              <parameter>src_w</parameter> and <parameter>src_h</parameter>
              parameters (as 16.16 fixed point values). Devices that don't support
              subpixel plane coordinates can ignore the fractional part.
            </para>
            <para>
              The destination rectangle in CRTC coordinates is given by the
              <parameter>crtc_x</parameter>, <parameter>crtc_y</parameter>,
              <parameter>crtc_w</parameter> and <parameter>crtc_h</parameter>
              parameters (as integer values). Devices scale the source rectangle to
              the destination rectangle. If scaling is not supported, and the source
              rectangle size doesn't match the destination rectangle size, the
              driver must return a -<errorname>EINVAL</errorname> error.
            </para>
          </listitem>
          <listitem>
            <synopsis>int (*disable_plane)(struct drm_plane *plane);</synopsis>
            <para>
              Disable the plane. The DRM core calls this method in response to a
              DRM_IOCTL_MODE_SETPLANE ioctl call with the frame buffer ID set to 0.
              Disabled planes must not be processed by the CRTC.
            </para>
          </listitem>
          <listitem>
            <synopsis>void (*destroy)(struct drm_plane *plane);</synopsis>
            <para>
              Destroy the plane when not needed anymore. See
              <xref linkend="drm-kms-init"/>.
            </para>
          </listitem>
        </itemizedlist>
      </sect3>
    </sect2>
    <sect2>
      <title>Encoders (struct <structname>drm_encoder</structname>)</title>
      <para>
        An encoder takes pixel data from a CRTC and converts it to a format
	suitable for any attached connectors. On some devices, it may be
	possible to have a CRTC send data to more than one encoder. In that
	case, both encoders would receive data from the same scanout buffer,
	resulting in a "cloned" display configuration across the connectors
	attached to each encoder.
      </para>
      <sect3>
        <title>Encoder Initialization</title>
        <para>
          As for CRTCs, a KMS driver must create, initialize and register at
          least one struct <structname>drm_encoder</structname> instance. The
          instance is allocated and zeroed by the driver, possibly as part of a
          larger structure.
        </para>
        <para>
          Drivers must initialize the struct <structname>drm_encoder</structname>
          <structfield>possible_crtcs</structfield> and
          <structfield>possible_clones</structfield> fields before registering the
          encoder. Both fields are bitmasks of respectively the CRTCs that the
          encoder can be connected to, and sibling encoders candidate for cloning.
        </para>
        <para>
          After being initialized, the encoder must be registered with a call to
          <function>drm_encoder_init</function>. The function takes a pointer to
          the encoder functions and an encoder type. Supported types are
          <itemizedlist>
            <listitem>
              DRM_MODE_ENCODER_DAC for VGA and analog on DVI-I/DVI-A
              </listitem>
            <listitem>
              DRM_MODE_ENCODER_TMDS for DVI, HDMI and (embedded) DisplayPort
            </listitem>
            <listitem>
              DRM_MODE_ENCODER_LVDS for display panels
            </listitem>
            <listitem>
              DRM_MODE_ENCODER_TVDAC for TV output (Composite, S-Video, Component,
              SCART)
            </listitem>
            <listitem>
              DRM_MODE_ENCODER_VIRTUAL for virtual machine displays
            </listitem>
          </itemizedlist>
        </para>
        <para>
          Encoders must be attached to a CRTC to be used. DRM drivers leave
          encoders unattached at initialization time. Applications (or the fbdev
          compatibility layer when implemented) are responsible for attaching the
          encoders they want to use to a CRTC.
        </para>
      </sect3>
      <sect3>
        <title>Encoder Operations</title>
        <itemizedlist>
          <listitem>
            <synopsis>void (*destroy)(struct drm_encoder *encoder);</synopsis>
            <para>
              Called to destroy the encoder when not needed anymore. See
              <xref linkend="drm-kms-init"/>.
            </para>
          </listitem>
          <listitem>
            <synopsis>void (*set_property)(struct drm_plane *plane,
                     struct drm_property *property, uint64_t value);</synopsis>
            <para>
              Set the value of the given plane property to
              <parameter>value</parameter>. See <xref linkend="drm-kms-properties"/>
              for more information about properties.
            </para>
          </listitem>
        </itemizedlist>
      </sect3>
    </sect2>
    <sect2>
      <title>Connectors (struct <structname>drm_connector</structname>)</title>
      <para>
        A connector is the final destination for pixel data on a device, and
	usually connects directly to an external display device like a monitor
	or laptop panel. A connector can only be attached to one encoder at a
	time. The connector is also the structure where information about the
	attached display is kept, so it contains fields for display data, EDID
	data, DPMS &amp; connection status, and information about modes
	supported on the attached displays.
      </para>
      <sect3>
        <title>Connector Initialization</title>
        <para>
          Finally a KMS driver must create, initialize, register and attach at
          least one struct <structname>drm_connector</structname> instance. The
          instance is created as other KMS objects and initialized by setting the
          following fields.
        </para>
        <variablelist>
          <varlistentry>
            <term><structfield>interlace_allowed</structfield></term>
            <listitem><para>
              Whether the connector can handle interlaced modes.
            </para></listitem>
          </varlistentry>
          <varlistentry>
            <term><structfield>doublescan_allowed</structfield></term>
            <listitem><para>
              Whether the connector can handle doublescan.
            </para></listitem>
          </varlistentry>
          <varlistentry>
            <term><structfield>display_info
            </structfield></term>
            <listitem><para>
              Display information is filled from EDID information when a display
              is detected. For non hot-pluggable displays such as flat panels in
              embedded systems, the driver should initialize the
              <structfield>display_info</structfield>.<structfield>width_mm</structfield>
              and
              <structfield>display_info</structfield>.<structfield>height_mm</structfield>
              fields with the physical size of the display.
            </para></listitem>
          </varlistentry>
          <varlistentry>
            <term id="drm-kms-connector-polled"><structfield>polled</structfield></term>
            <listitem><para>
              Connector polling mode, a combination of
              <variablelist>
                <varlistentry>
                  <term>DRM_CONNECTOR_POLL_HPD</term>
                  <listitem><para>
                    The connector generates hotplug events and doesn't need to be
                    periodically polled. The CONNECT and DISCONNECT flags must not
                    be set together with the HPD flag.
                  </para></listitem>
                </varlistentry>
                <varlistentry>
                  <term>DRM_CONNECTOR_POLL_CONNECT</term>
                  <listitem><para>
                    Periodically poll the connector for connection.
                  </para></listitem>
                </varlistentry>
                <varlistentry>
                  <term>DRM_CONNECTOR_POLL_DISCONNECT</term>
                  <listitem><para>
                    Periodically poll the connector for disconnection.
                  </para></listitem>
                </varlistentry>
              </variablelist>
              Set to 0 for connectors that don't support connection status
              discovery.
            </para></listitem>
          </varlistentry>
        </variablelist>
        <para>
          The connector is then registered with a call to
          <function>drm_connector_init</function> with a pointer to the connector
          functions and a connector type, and exposed through sysfs with a call to
          <function>drm_connector_register</function>.
        </para>
        <para>
          Supported connector types are
          <itemizedlist>
            <listitem>DRM_MODE_CONNECTOR_VGA</listitem>
            <listitem>DRM_MODE_CONNECTOR_DVII</listitem>
            <listitem>DRM_MODE_CONNECTOR_DVID</listitem>
            <listitem>DRM_MODE_CONNECTOR_DVIA</listitem>
            <listitem>DRM_MODE_CONNECTOR_Composite</listitem>
            <listitem>DRM_MODE_CONNECTOR_SVIDEO</listitem>
            <listitem>DRM_MODE_CONNECTOR_LVDS</listitem>
            <listitem>DRM_MODE_CONNECTOR_Component</listitem>
            <listitem>DRM_MODE_CONNECTOR_9PinDIN</listitem>
            <listitem>DRM_MODE_CONNECTOR_DisplayPort</listitem>
            <listitem>DRM_MODE_CONNECTOR_HDMIA</listitem>
            <listitem>DRM_MODE_CONNECTOR_HDMIB</listitem>
            <listitem>DRM_MODE_CONNECTOR_TV</listitem>
            <listitem>DRM_MODE_CONNECTOR_eDP</listitem>
            <listitem>DRM_MODE_CONNECTOR_VIRTUAL</listitem>
          </itemizedlist>
        </para>
        <para>
          Connectors must be attached to an encoder to be used. For devices that
          map connectors to encoders 1:1, the connector should be attached at
          initialization time with a call to
          <function>drm_mode_connector_attach_encoder</function>. The driver must
          also set the <structname>drm_connector</structname>
          <structfield>encoder</structfield> field to point to the attached
          encoder.
        </para>
        <para>
          Finally, drivers must initialize the connectors state change detection
          with a call to <function>drm_kms_helper_poll_init</function>. If at
          least one connector is pollable but can't generate hotplug interrupts
          (indicated by the DRM_CONNECTOR_POLL_CONNECT and
          DRM_CONNECTOR_POLL_DISCONNECT connector flags), a delayed work will
          automatically be queued to periodically poll for changes. Connectors
          that can generate hotplug interrupts must be marked with the
          DRM_CONNECTOR_POLL_HPD flag instead, and their interrupt handler must
          call <function>drm_helper_hpd_irq_event</function>. The function will
          queue a delayed work to check the state of all connectors, but no
          periodic polling will be done.
        </para>
      </sect3>
      <sect3>
        <title>Connector Operations</title>
        <note><para>
          Unless otherwise state, all operations are mandatory.
        </para></note>
        <sect4>
          <title>DPMS</title>
          <synopsis>void (*dpms)(struct drm_connector *connector, int mode);</synopsis>
          <para>
            The DPMS operation sets the power state of a connector. The mode
            argument is one of
            <itemizedlist>
              <listitem><para>DRM_MODE_DPMS_ON</para></listitem>
              <listitem><para>DRM_MODE_DPMS_STANDBY</para></listitem>
              <listitem><para>DRM_MODE_DPMS_SUSPEND</para></listitem>
              <listitem><para>DRM_MODE_DPMS_OFF</para></listitem>
            </itemizedlist>
          </para>
          <para>
            In all but DPMS_ON mode the encoder to which the connector is attached
            should put the display in low-power mode by driving its signals
            appropriately. If more than one connector is attached to the encoder
            care should be taken not to change the power state of other displays as
            a side effect. Low-power mode should be propagated to the encoders and
            CRTCs when all related connectors are put in low-power mode.
          </para>
        </sect4>
        <sect4>
          <title>Modes</title>
          <synopsis>int (*fill_modes)(struct drm_connector *connector, uint32_t max_width,
                      uint32_t max_height);</synopsis>
          <para>
            Fill the mode list with all supported modes for the connector. If the
            <parameter>max_width</parameter> and <parameter>max_height</parameter>
            arguments are non-zero, the implementation must ignore all modes wider
            than <parameter>max_width</parameter> or higher than
            <parameter>max_height</parameter>.
          </para>
          <para>
            The connector must also fill in this operation its
            <structfield>display_info</structfield>
            <structfield>width_mm</structfield> and
            <structfield>height_mm</structfield> fields with the connected display
            physical size in millimeters. The fields should be set to 0 if the value
            isn't known or is not applicable (for instance for projector devices).
          </para>
        </sect4>
        <sect4>
          <title>Connection Status</title>
          <para>
            The connection status is updated through polling or hotplug events when
            supported (see <xref linkend="drm-kms-connector-polled"/>). The status
            value is reported to userspace through ioctls and must not be used
            inside the driver, as it only gets initialized by a call to
            <function>drm_mode_getconnector</function> from userspace.
          </para>
          <synopsis>enum drm_connector_status (*detect)(struct drm_connector *connector,
                                        bool force);</synopsis>
          <para>
            Check to see if anything is attached to the connector. The
            <parameter>force</parameter> parameter is set to false whilst polling or
            to true when checking the connector due to user request.
            <parameter>force</parameter> can be used by the driver to avoid
            expensive, destructive operations during automated probing.
          </para>
          <para>
            Return connector_status_connected if something is connected to the
            connector, connector_status_disconnected if nothing is connected and
            connector_status_unknown if the connection state isn't known.
          </para>
          <para>
            Drivers should only return connector_status_connected if the connection
            status has really been probed as connected. Connectors that can't detect
            the connection status, or failed connection status probes, should return
            connector_status_unknown.
          </para>
        </sect4>
        <sect4>
          <title>Miscellaneous</title>
          <itemizedlist>
            <listitem>
              <synopsis>void (*set_property)(struct drm_connector *connector,
                     struct drm_property *property, uint64_t value);</synopsis>
              <para>
                Set the value of the given connector property to
                <parameter>value</parameter>. See <xref linkend="drm-kms-properties"/>
                for more information about properties.
              </para>
            </listitem>
            <listitem>
              <synopsis>void (*destroy)(struct drm_connector *connector);</synopsis>
              <para>
                Destroy the connector when not needed anymore. See
                <xref linkend="drm-kms-init"/>.
              </para>
            </listitem>
          </itemizedlist>
        </sect4>
      </sect3>
    </sect2>
    <sect2>
      <title>Cleanup</title>
      <para>
        The DRM core manages its objects' lifetime. When an object is not needed
	anymore the core calls its destroy function, which must clean up and
	free every resource allocated for the object. Every
	<function>drm_*_init</function> call must be matched with a
	corresponding <function>drm_*_cleanup</function> call to cleanup CRTCs
	(<function>drm_crtc_cleanup</function>), planes
	(<function>drm_plane_cleanup</function>), encoders
	(<function>drm_encoder_cleanup</function>) and connectors
	(<function>drm_connector_cleanup</function>). Furthermore, connectors
	that have been added to sysfs must be removed by a call to
	<function>drm_connector_unregister</function> before calling
	<function>drm_connector_cleanup</function>.
      </para>
      <para>
        Connectors state change detection must be cleanup up with a call to
	<function>drm_kms_helper_poll_fini</function>.
      </para>
    </sect2>
    <sect2>
      <title>Output discovery and initialization example</title>
      <programlisting><![CDATA[
void intel_crt_init(struct drm_device *dev)
{
	struct drm_connector *connector;
	struct intel_output *intel_output;

	intel_output = kzalloc(sizeof(struct intel_output), GFP_KERNEL);
	if (!intel_output)
		return;

	connector = &intel_output->base;
	drm_connector_init(dev, &intel_output->base,
			   &intel_crt_connector_funcs, DRM_MODE_CONNECTOR_VGA);

	drm_encoder_init(dev, &intel_output->enc, &intel_crt_enc_funcs,
			 DRM_MODE_ENCODER_DAC);

	drm_mode_connector_attach_encoder(&intel_output->base,
					  &intel_output->enc);

	/* Set up the DDC bus. */
	intel_output->ddc_bus = intel_i2c_create(dev, GPIOA, "CRTDDC_A");
	if (!intel_output->ddc_bus) {
		dev_printk(KERN_ERR, &dev->pdev->dev, "DDC bus registration "
			   "failed.\n");
		return;
	}

	intel_output->type = INTEL_OUTPUT_ANALOG;
	connector->interlace_allowed = 0;
	connector->doublescan_allowed = 0;

	drm_encoder_helper_add(&intel_output->enc, &intel_crt_helper_funcs);
	drm_connector_helper_add(connector, &intel_crt_connector_helper_funcs);

	drm_connector_register(connector);
}]]></programlisting>
      <para>
        In the example above (taken from the i915 driver), a CRTC, connector and
        encoder combination is created. A device-specific i2c bus is also
        created for fetching EDID data and performing monitor detection. Once
        the process is complete, the new connector is registered with sysfs to
        make its properties available to applications.
      </para>
    </sect2>
    <sect2>
      <title>KMS API Functions</title>
!Edrivers/gpu/drm/drm_crtc.c
    </sect2>
    <sect2>
      <title>KMS Data Structures</title>
!Iinclude/drm/drm_crtc.h
    </sect2>
    <sect2>
      <title>KMS Locking</title>
!Pdrivers/gpu/drm/drm_modeset_lock.c kms locking
!Iinclude/drm/drm_modeset_lock.h
!Edrivers/gpu/drm/drm_modeset_lock.c
    </sect2>
  </sect1>

  <!-- Internals: kms helper functions -->

  <sect1>
    <title>Mode Setting Helper Functions</title>
    <para>
      The plane, CRTC, encoder and connector functions provided by the drivers
      implement the DRM API. They're called by the DRM core and ioctl handlers
      to handle device state changes and configuration request. As implementing
      those functions often requires logic not specific to drivers, mid-layer
      helper functions are available to avoid duplicating boilerplate code.
    </para>
    <para>
      The DRM core contains one mid-layer implementation. The mid-layer provides
      implementations of several plane, CRTC, encoder and connector functions
      (called from the top of the mid-layer) that pre-process requests and call
      lower-level functions provided by the driver (at the bottom of the
      mid-layer). For instance, the
      <function>drm_crtc_helper_set_config</function> function can be used to
      fill the struct <structname>drm_crtc_funcs</structname>
      <structfield>set_config</structfield> field. When called, it will split
      the <methodname>set_config</methodname> operation in smaller, simpler
      operations and call the driver to handle them.
    </para>
    <para>
      To use the mid-layer, drivers call <function>drm_crtc_helper_add</function>,
      <function>drm_encoder_helper_add</function> and
      <function>drm_connector_helper_add</function> functions to install their
      mid-layer bottom operations handlers, and fill the
      <structname>drm_crtc_funcs</structname>,
      <structname>drm_encoder_funcs</structname> and
      <structname>drm_connector_funcs</structname> structures with pointers to
      the mid-layer top API functions. Installing the mid-layer bottom operation
      handlers is best done right after registering the corresponding KMS object.
    </para>
    <para>
      The mid-layer is not split between CRTC, encoder and connector operations.
      To use it, a driver must provide bottom functions for all of the three KMS
      entities.
    </para>
    <sect2>
      <title>Helper Functions</title>
      <itemizedlist>
        <listitem>
          <synopsis>int drm_crtc_helper_set_config(struct drm_mode_set *set);</synopsis>
          <para>
            The <function>drm_crtc_helper_set_config</function> helper function
            is a CRTC <methodname>set_config</methodname> implementation. It
            first tries to locate the best encoder for each connector by calling
            the connector <methodname>best_encoder</methodname> helper
            operation.
          </para>
          <para>
            After locating the appropriate encoders, the helper function will
            call the <methodname>mode_fixup</methodname> encoder and CRTC helper
            operations to adjust the requested mode, or reject it completely in
            which case an error will be returned to the application. If the new
            configuration after mode adjustment is identical to the current
            configuration the helper function will return without performing any
            other operation.
          </para>
          <para>
            If the adjusted mode is identical to the current mode but changes to
            the frame buffer need to be applied, the
            <function>drm_crtc_helper_set_config</function> function will call
            the CRTC <methodname>mode_set_base</methodname> helper operation. If
            the adjusted mode differs from the current mode, or if the
            <methodname>mode_set_base</methodname> helper operation is not
            provided, the helper function performs a full mode set sequence by
            calling the <methodname>prepare</methodname>,
            <methodname>mode_set</methodname> and
            <methodname>commit</methodname> CRTC and encoder helper operations,
            in that order.
          </para>
        </listitem>
        <listitem>
          <synopsis>void drm_helper_connector_dpms(struct drm_connector *connector, int mode);</synopsis>
          <para>
            The <function>drm_helper_connector_dpms</function> helper function
            is a connector <methodname>dpms</methodname> implementation that
            tracks power state of connectors. To use the function, drivers must
            provide <methodname>dpms</methodname> helper operations for CRTCs
            and encoders to apply the DPMS state to the device.
          </para>
          <para>
            The mid-layer doesn't track the power state of CRTCs and encoders.
            The <methodname>dpms</methodname> helper operations can thus be
            called with a mode identical to the currently active mode.
          </para>
        </listitem>
        <listitem>
          <synopsis>int drm_helper_probe_single_connector_modes(struct drm_connector *connector,
                                            uint32_t maxX, uint32_t maxY);</synopsis>
          <para>
            The <function>drm_helper_probe_single_connector_modes</function> helper
            function is a connector <methodname>fill_modes</methodname>
            implementation that updates the connection status for the connector
            and then retrieves a list of modes by calling the connector
            <methodname>get_modes</methodname> helper operation.
          </para>
         <para>
            If the helper operation returns no mode, and if the connector status
            is connector_status_connected, standard VESA DMT modes up to
            1024x768 are automatically added to the modes list by a call to
            <function>drm_add_modes_noedid</function>.
          </para>
          <para>
            The function then filters out modes larger than
            <parameter>max_width</parameter> and <parameter>max_height</parameter>
            if specified. It finally calls the optional connector
            <methodname>mode_valid</methodname> helper operation for each mode in
            the probed list to check whether the mode is valid for the connector.
          </para>
        </listitem>
      </itemizedlist>
    </sect2>
    <sect2>
      <title>CRTC Helper Operations</title>
      <itemizedlist>
        <listitem id="drm-helper-crtc-mode-fixup">
          <synopsis>bool (*mode_fixup)(struct drm_crtc *crtc,
                       const struct drm_display_mode *mode,
                       struct drm_display_mode *adjusted_mode);</synopsis>
          <para>
            Let CRTCs adjust the requested mode or reject it completely. This
            operation returns true if the mode is accepted (possibly after being
            adjusted) or false if it is rejected.
          </para>
          <para>
            The <methodname>mode_fixup</methodname> operation should reject the
            mode if it can't reasonably use it. The definition of "reasonable"
            is currently fuzzy in this context. One possible behaviour would be
            to set the adjusted mode to the panel timings when a fixed-mode
            panel is used with hardware capable of scaling. Another behaviour
            would be to accept any input mode and adjust it to the closest mode
            supported by the hardware (FIXME: This needs to be clarified).
          </para>
        </listitem>
        <listitem>
          <synopsis>int (*mode_set_base)(struct drm_crtc *crtc, int x, int y,
                     struct drm_framebuffer *old_fb)</synopsis>
          <para>
            Move the CRTC on the current frame buffer (stored in
            <literal>crtc-&gt;fb</literal>) to position (x,y). Any of the frame
            buffer, x position or y position may have been modified.
          </para>
          <para>
            This helper operation is optional. If not provided, the
            <function>drm_crtc_helper_set_config</function> function will fall
            back to the <methodname>mode_set</methodname> helper operation.
          </para>
          <note><para>
            FIXME: Why are x and y passed as arguments, as they can be accessed
            through <literal>crtc-&gt;x</literal> and
            <literal>crtc-&gt;y</literal>?
          </para></note>
        </listitem>
        <listitem>
          <synopsis>void (*prepare)(struct drm_crtc *crtc);</synopsis>
          <para>
            Prepare the CRTC for mode setting. This operation is called after
            validating the requested mode. Drivers use it to perform
            device-specific operations required before setting the new mode.
          </para>
        </listitem>
        <listitem>
          <synopsis>int (*mode_set)(struct drm_crtc *crtc, struct drm_display_mode *mode,
                struct drm_display_mode *adjusted_mode, int x, int y,
                struct drm_framebuffer *old_fb);</synopsis>
          <para>
            Set a new mode, position and frame buffer. Depending on the device
            requirements, the mode can be stored internally by the driver and
            applied in the <methodname>commit</methodname> operation, or
            programmed to the hardware immediately.
          </para>
          <para>
            The <methodname>mode_set</methodname> operation returns 0 on success
	    or a negative error code if an error occurs.
          </para>
        </listitem>
        <listitem>
          <synopsis>void (*commit)(struct drm_crtc *crtc);</synopsis>
          <para>
            Commit a mode. This operation is called after setting the new mode.
            Upon return the device must use the new mode and be fully
            operational.
          </para>
        </listitem>
      </itemizedlist>
    </sect2>
    <sect2>
      <title>Encoder Helper Operations</title>
      <itemizedlist>
        <listitem>
          <synopsis>bool (*mode_fixup)(struct drm_encoder *encoder,
                       const struct drm_display_mode *mode,
                       struct drm_display_mode *adjusted_mode);</synopsis>
          <para>
            Let encoders adjust the requested mode or reject it completely. This
            operation returns true if the mode is accepted (possibly after being
            adjusted) or false if it is rejected. See the
            <link linkend="drm-helper-crtc-mode-fixup">mode_fixup CRTC helper
            operation</link> for an explanation of the allowed adjustments.
          </para>
        </listitem>
        <listitem>
          <synopsis>void (*prepare)(struct drm_encoder *encoder);</synopsis>
          <para>
            Prepare the encoder for mode setting. This operation is called after
            validating the requested mode. Drivers use it to perform
            device-specific operations required before setting the new mode.
          </para>
        </listitem>
        <listitem>
          <synopsis>void (*mode_set)(struct drm_encoder *encoder,
                 struct drm_display_mode *mode,
                 struct drm_display_mode *adjusted_mode);</synopsis>
          <para>
            Set a new mode. Depending on the device requirements, the mode can
            be stored internally by the driver and applied in the
            <methodname>commit</methodname> operation, or programmed to the
            hardware immediately.
          </para>
        </listitem>
        <listitem>
          <synopsis>void (*commit)(struct drm_encoder *encoder);</synopsis>
          <para>
            Commit a mode. This operation is called after setting the new mode.
            Upon return the device must use the new mode and be fully
            operational.
          </para>
        </listitem>
      </itemizedlist>
    </sect2>
    <sect2>
      <title>Connector Helper Operations</title>
      <itemizedlist>
        <listitem>
          <synopsis>struct drm_encoder *(*best_encoder)(struct drm_connector *connector);</synopsis>
          <para>
            Return a pointer to the best encoder for the connecter. Device that
            map connectors to encoders 1:1 simply return the pointer to the
            associated encoder. This operation is mandatory.
          </para>
        </listitem>
        <listitem>
          <synopsis>int (*get_modes)(struct drm_connector *connector);</synopsis>
          <para>
            Fill the connector's <structfield>probed_modes</structfield> list
            by parsing EDID data with <function>drm_add_edid_modes</function>,
            adding standard VESA DMT modes with <function>drm_add_modes_noedid</function>,
            or calling <function>drm_mode_probed_add</function> directly for every
            supported mode and return the number of modes it has detected. This
            operation is mandatory.
          </para>
          <para>
            Note that the caller function will automatically add standard VESA
            DMT modes up to 1024x768 if the <methodname>get_modes</methodname>
            helper operation returns no mode and if the connector status is
            connector_status_connected. There is no need to call
            <function>drm_add_edid_modes</function> manually in that case.
          </para>
          <para>
            When adding modes manually the driver creates each mode with a call to
            <function>drm_mode_create</function> and must fill the following fields.
            <itemizedlist>
              <listitem>
                <synopsis>__u32 type;</synopsis>
                <para>
                  Mode type bitmask, a combination of
                  <variablelist>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_BUILTIN</term>
                      <listitem><para>not used?</para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_CLOCK_C</term>
                      <listitem><para>not used?</para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_CRTC_C</term>
                      <listitem><para>not used?</para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>
        DRM_MODE_TYPE_PREFERRED - The preferred mode for the connector
                      </term>
                      <listitem>
                        <para>not used?</para>
                      </listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_DEFAULT</term>
                      <listitem><para>not used?</para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_USERDEF</term>
                      <listitem><para>not used?</para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_TYPE_DRIVER</term>
                      <listitem>
                        <para>
                          The mode has been created by the driver (as opposed to
                          to user-created modes).
                        </para>
                      </listitem>
                    </varlistentry>
                  </variablelist>
                  Drivers must set the DRM_MODE_TYPE_DRIVER bit for all modes they
                  create, and set the DRM_MODE_TYPE_PREFERRED bit for the preferred
                  mode.
                </para>
              </listitem>
              <listitem>
                <synopsis>__u32 clock;</synopsis>
                <para>Pixel clock frequency in kHz unit</para>
              </listitem>
              <listitem>
                <synopsis>__u16 hdisplay, hsync_start, hsync_end, htotal;
    __u16 vdisplay, vsync_start, vsync_end, vtotal;</synopsis>
                <para>Horizontal and vertical timing information</para>
                <screen><![CDATA[
             Active                 Front           Sync           Back
             Region                 Porch                          Porch
    <-----------------------><----------------><-------------><-------------->

      //////////////////////|
     ////////////////////// |
    //////////////////////  |..................               ................
                                               _______________

    <----- [hv]display ----->
    <------------- [hv]sync_start ------------>
    <--------------------- [hv]sync_end --------------------->
    <-------------------------------- [hv]total ----------------------------->
]]></screen>
              </listitem>
              <listitem>
                <synopsis>__u16 hskew;
    __u16 vscan;</synopsis>
                <para>Unknown</para>
              </listitem>
              <listitem>
                <synopsis>__u32 flags;</synopsis>
                <para>
                  Mode flags, a combination of
                  <variablelist>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_PHSYNC</term>
                      <listitem><para>
                        Horizontal sync is active high
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_NHSYNC</term>
                      <listitem><para>
                        Horizontal sync is active low
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_PVSYNC</term>
                      <listitem><para>
                        Vertical sync is active high
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_NVSYNC</term>
                      <listitem><para>
                        Vertical sync is active low
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_INTERLACE</term>
                      <listitem><para>
                        Mode is interlaced
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_DBLSCAN</term>
                      <listitem><para>
                        Mode uses doublescan
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_CSYNC</term>
                      <listitem><para>
                        Mode uses composite sync
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_PCSYNC</term>
                      <listitem><para>
                        Composite sync is active high
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_NCSYNC</term>
                      <listitem><para>
                        Composite sync is active low
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_HSKEW</term>
                      <listitem><para>
                        hskew provided (not used?)
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_BCAST</term>
                      <listitem><para>
                        not used?
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_PIXMUX</term>
                      <listitem><para>
                        not used?
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_DBLCLK</term>
                      <listitem><para>
                        not used?
                      </para></listitem>
                    </varlistentry>
                    <varlistentry>
                      <term>DRM_MODE_FLAG_CLKDIV2</term>
                      <listitem><para>
                        ?
                      </para></listitem>
                    </varlistentry>
                  </variablelist>
                </para>
                <para>
                  Note that modes marked with the INTERLACE or DBLSCAN flags will be
                  filtered out by
                  <function>drm_helper_probe_single_connector_modes</function> if
                  the connector's <structfield>interlace_allowed</structfield> or
                  <structfield>doublescan_allowed</structfield> field is set to 0.
                </para>
              </listitem>
              <listitem>
                <synopsis>char name[DRM_DISPLAY_MODE_LEN];</synopsis>
                <para>
                  Mode name. The driver must call
                  <function>drm_mode_set_name</function> to fill the mode name from
                  <structfield>hdisplay</structfield>,
                  <structfield>vdisplay</structfield> and interlace flag after
                  filling the corresponding fields.
                </para>
              </listitem>
            </itemizedlist>
          </para>
          <para>
            The <structfield>vrefresh</structfield> value is computed by
            <function>drm_helper_probe_single_connector_modes</function>.
          </para>
          <para>
            When parsing EDID data, <function>drm_add_edid_modes</function> fills the
            connector <structfield>display_info</structfield>
            <structfield>width_mm</structfield> and
            <structfield>height_mm</structfield> fields. When creating modes
            manually the <methodname>get_modes</methodname> helper operation must
            set the <structfield>display_info</structfield>
            <structfield>width_mm</structfield> and
            <structfield>height_mm</structfield> fields if they haven't been set
            already (for instance at initialization time when a fixed-size panel is
            attached to the connector). The mode <structfield>width_mm</structfield>
            and <structfield>height_mm</structfield> fields are only used internally
            during EDID parsing and should not be set when creating modes manually.
          </para>
        </listitem>
        <listitem>
          <synopsis>int (*mode_valid)(struct drm_connector *connector,
		  struct drm_display_mode *mode);</synopsis>
          <para>
            Verify whether a mode is valid for the connector. Return MODE_OK for
            supported modes and one of the enum drm_mode_status values (MODE_*)
            for unsupported modes. This operation is optional.
          </para>
          <para>
            As the mode rejection reason is currently not used beside for
            immediately removing the unsupported mode, an implementation can
            return MODE_BAD regardless of the exact reason why the mode is not
            valid.
          </para>
          <note><para>
            Note that the <methodname>mode_valid</methodname> helper operation is
            only called for modes detected by the device, and
            <emphasis>not</emphasis> for modes set by the user through the CRTC
            <methodname>set_config</methodname> operation.
          </para></note>
        </listitem>
      </itemizedlist>
    </sect2>
    <sect2>
      <title>Atomic Modeset Helper Functions Reference</title>
      <sect3>
	<title>Overview</title>
!Pdrivers/gpu/drm/drm_atomic_helper.c overview
      </sect3>
      <sect3>
	<title>Implementing Asynchronous Atomic Commit</title>
!Pdrivers/gpu/drm/drm_atomic_helper.c implementing async commit
      </sect3>
      <sect3>
	<title>Atomic State Reset and Initialization</title>
!Pdrivers/gpu/drm/drm_atomic_helper.c atomic state reset and initialization
      </sect3>
!Iinclude/drm/drm_atomic_helper.h
!Edrivers/gpu/drm/drm_atomic_helper.c
    </sect2>
    <sect2>
      <title>Modeset Helper Functions Reference</title>
!Iinclude/drm/drm_crtc_helper.h
!Edrivers/gpu/drm/drm_crtc_helper.c
!Pdrivers/gpu/drm/drm_crtc_helper.c overview
    </sect2>
    <sect2>
      <title>Output Probing Helper Functions Reference</title>
!Pdrivers/gpu/drm/drm_probe_helper.c output probing helper overview
!Edrivers/gpu/drm/drm_probe_helper.c
    </sect2>
    <sect2>
      <title>fbdev Helper Functions Reference</title>
!Pdrivers/gpu/drm/drm_fb_helper.c fbdev helpers
!Edrivers/gpu/drm/drm_fb_helper.c
!Iinclude/drm/drm_fb_helper.h
    </sect2>
    <sect2>
      <title>Display Port Helper Functions Reference</title>
!Pdrivers/gpu/drm/drm_dp_helper.c dp helpers
!Iinclude/drm/drm_dp_helper.h
!Edrivers/gpu/drm/drm_dp_helper.c
    </sect2>
    <sect2>
      <title>Display Port MST Helper Functions Reference</title>
!Pdrivers/gpu/drm/drm_dp_mst_topology.c dp mst helper
!Iinclude/drm/drm_dp_mst_helper.h
!Edrivers/gpu/drm/drm_dp_mst_topology.c
    </sect2>
    <sect2>
      <title>MIPI DSI Helper Functions Reference</title>
!Pdrivers/gpu/drm/drm_mipi_dsi.c dsi helpers
!Iinclude/drm/drm_mipi_dsi.h
!Edrivers/gpu/drm/drm_mipi_dsi.c
    </sect2>
    <sect2>
      <title>EDID Helper Functions Reference</title>
!Edrivers/gpu/drm/drm_edid.c
    </sect2>
    <sect2>
      <title>Rectangle Utilities Reference</title>
!Pinclude/drm/drm_rect.h rect utils
!Iinclude/drm/drm_rect.h
!Edrivers/gpu/drm/drm_rect.c
    </sect2>
    <sect2>
      <title>Flip-work Helper Reference</title>
!Pinclude/drm/drm_flip_work.h flip utils
!Iinclude/drm/drm_flip_work.h
!Edrivers/gpu/drm/drm_flip_work.c
    </sect2>
    <sect2>
      <title>HDMI Infoframes Helper Reference</title>
      <para>
	Strictly speaking this is not a DRM helper library but generally useable
	by any driver interfacing with HDMI outputs like v4l or alsa drivers.
	But it nicely fits into the overall topic of mode setting helper
	libraries and hence is also included here.
      </para>
!Iinclude/linux/hdmi.h
!Edrivers/video/hdmi.c
    </sect2>
    <sect2>
      <title id="drm-kms-planehelpers">Plane Helper Reference</title>
!Edrivers/gpu/drm/drm_plane_helper.c
!Pdrivers/gpu/drm/drm_plane_helper.c overview
    </sect2>
    <sect2>
	  <title>Tile group</title>
!Pdrivers/gpu/drm/drm_crtc.c Tile group
    </sect2>
    <sect2>
	<title>Bridges</title>
      <sect3>
	 <title>Overview</title>
!Pdrivers/gpu/drm/drm_bridge.c overview
      </sect3>
      <sect3>
	 <title>Default bridge callback sequence</title>
!Pdrivers/gpu/drm/drm_bridge.c bridge callbacks
      </sect3>
!Edrivers/gpu/drm/drm_bridge.c
    </sect2>
  </sect1>

  <!-- Internals: kms properties -->

  <sect1 id="drm-kms-properties">
    <title>KMS Properties</title>
    <para>
      Drivers may need to expose additional parameters to applications than
      those described in the previous sections. KMS supports attaching
      properties to CRTCs, connectors and planes and offers a userspace API to
      list, get and set the property values.
    </para>
    <para>
      Properties are identified by a name that uniquely defines the property
      purpose, and store an associated value. For all property types except blob
      properties the value is a 64-bit unsigned integer.
    </para>
    <para>
      KMS differentiates between properties and property instances. Drivers
      first create properties and then create and associate individual instances
      of those properties to objects. A property can be instantiated multiple
      times and associated with different objects. Values are stored in property
      instances, and all other property information are stored in the property
      and shared between all instances of the property.
    </para>
    <para>
      Every property is created with a type that influences how the KMS core
      handles the property. Supported property types are
      <variablelist>
        <varlistentry>
          <term>DRM_MODE_PROP_RANGE</term>
          <listitem><para>Range properties report their minimum and maximum
            admissible values. The KMS core verifies that values set by
            application fit in that range.</para></listitem>
        </varlistentry>
        <varlistentry>
          <term>DRM_MODE_PROP_ENUM</term>
          <listitem><para>Enumerated properties take a numerical value that
            ranges from 0 to the number of enumerated values defined by the
            property minus one, and associate a free-formed string name to each
            value. Applications can retrieve the list of defined value-name pairs
            and use the numerical value to get and set property instance values.
            </para></listitem>
        </varlistentry>
        <varlistentry>
          <term>DRM_MODE_PROP_BITMASK</term>
          <listitem><para>Bitmask properties are enumeration properties that
            additionally restrict all enumerated values to the 0..63 range.
            Bitmask property instance values combine one or more of the
            enumerated bits defined by the property.</para></listitem>
        </varlistentry>
        <varlistentry>
          <term>DRM_MODE_PROP_BLOB</term>
          <listitem><para>Blob properties store a binary blob without any format
            restriction. The binary blobs are created as KMS standalone objects,
            and blob property instance values store the ID of their associated
            blob object.</para>
	    <para>Blob properties are only used for the connector EDID property
	    and cannot be created by drivers.</para></listitem>
        </varlistentry>
      </variablelist>
    </para>
    <para>
      To create a property drivers call one of the following functions depending
      on the property type. All property creation functions take property flags
      and name, as well as type-specific arguments.
      <itemizedlist>
        <listitem>
          <synopsis>struct drm_property *drm_property_create_range(struct drm_device *dev, int flags,
                                               const char *name,
                                               uint64_t min, uint64_t max);</synopsis>
          <para>Create a range property with the given minimum and maximum
            values.</para>
        </listitem>
        <listitem>
          <synopsis>struct drm_property *drm_property_create_enum(struct drm_device *dev, int flags,
                                              const char *name,
                                              const struct drm_prop_enum_list *props,
                                              int num_values);</synopsis>
          <para>Create an enumerated property. The <parameter>props</parameter>
            argument points to an array of <parameter>num_values</parameter>
            value-name pairs.</para>
        </listitem>
        <listitem>
          <synopsis>struct drm_property *drm_property_create_bitmask(struct drm_device *dev,
                                                 int flags, const char *name,
                                                 const struct drm_prop_enum_list *props,
                                                 int num_values);</synopsis>
          <para>Create a bitmask property. The <parameter>props</parameter>
            argument points to an array of <parameter>num_values</parameter>
            value-name pairs.</para>
        </listitem>
      </itemizedlist>
    </para>
    <para>
      Properties can additionally be created as immutable, in which case they
      will be read-only for applications but can be modified by the driver. To
      create an immutable property drivers must set the DRM_MODE_PROP_IMMUTABLE
      flag at property creation time.
    </para>
    <para>
      When no array of value-name pairs is readily available at property
      creation time for enumerated or range properties, drivers can create
      the property using the <function>drm_property_create</function> function
      and manually add enumeration value-name pairs by calling the
      <function>drm_property_add_enum</function> function. Care must be taken to
      properly specify the property type through the <parameter>flags</parameter>
      argument.
    </para>
    <para>
      After creating properties drivers can attach property instances to CRTC,
      connector and plane objects by calling the
      <function>drm_object_attach_property</function>. The function takes a
      pointer to the target object, a pointer to the previously created property
      and an initial instance value.
    </para>
    <sect2>
	<title>Existing KMS Properties</title>
	<para>
	The following table gives description of drm properties exposed by various
	modules/drivers.
	</para>
	<table border="1" cellpadding="0" cellspacing="0">
	<tbody>
	<tr style="font-weight: bold;">
	<td valign="top" >Owner Module/Drivers</td>
	<td valign="top" >Group</td>
	<td valign="top" >Property Name</td>
	<td valign="top" >Type</td>
	<td valign="top" >Property Values</td>
	<td valign="top" >Object attached</td>
	<td valign="top" >Description/Restrictions</td>
	</tr>
	<tr>
	<td rowspan="37" valign="top" >DRM</td>
	<td valign="top" >Generic</td>
	<td valign="top" >“rotation”</td>
	<td valign="top" >BITMASK</td>
	<td valign="top" >{ 0, "rotate-0" },
	{ 1, "rotate-90" },
	{ 2, "rotate-180" },
	{ 3, "rotate-270" },
	{ 4, "reflect-x" },
	{ 5, "reflect-y" }</td>
	<td valign="top" >CRTC, Plane</td>
	<td valign="top" >rotate-(degrees) rotates the image by the specified amount in degrees
	in counter clockwise direction. reflect-x and reflect-y reflects the
	image along the specified axis prior to rotation</td>
	</tr>
	<tr>
	<td rowspan="5" valign="top" >Connector</td>
	<td valign="top" >“EDID”</td>
	<td valign="top" >BLOB | IMMUTABLE</td>
	<td valign="top" >0</td>
	<td valign="top" >Connector</td>
	<td valign="top" >Contains id of edid blob ptr object.</td>
	</tr>
	<tr>
	<td valign="top" >“DPMS”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ “On”, “Standby”, “Suspend”, “Off” }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >Contains DPMS operation mode value.</td>
	</tr>
	<tr>
	<td valign="top" >“PATH”</td>
	<td valign="top" >BLOB | IMMUTABLE</td>
	<td valign="top" >0</td>
	<td valign="top" >Connector</td>
	<td valign="top" >Contains topology path to a connector.</td>
	</tr>
	<tr>
	<td valign="top" >“TILE”</td>
	<td valign="top" >BLOB | IMMUTABLE</td>
	<td valign="top" >0</td>
	<td valign="top" >Connector</td>
	<td valign="top" >Contains tiling information for a connector.</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_ID”</td>
	<td valign="top" >OBJECT</td>
	<td valign="top" >DRM_MODE_OBJECT_CRTC</td>
	<td valign="top" >Connector</td>
	<td valign="top" >CRTC that connector is attached to (atomic)</td>
	</tr>
	<tr>
	<td rowspan="11" valign="top" >Plane</td>
	<td valign="top" >“type”</td>
	<td valign="top" >ENUM | IMMUTABLE</td>
	<td valign="top" >{ "Overlay", "Primary", "Cursor" }</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Plane type</td>
	</tr>
	<tr>
	<td valign="top" >“SRC_X”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout source x coordinate in 16.16 fixed point (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“SRC_Y”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout source y coordinate in 16.16 fixed point (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“SRC_W”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout source width in 16.16 fixed point (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“SRC_H”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout source height in 16.16 fixed point (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_X”</td>
	<td valign="top" >SIGNED_RANGE</td>
	<td valign="top" >Min=INT_MIN, Max=INT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout CRTC (destination) x coordinate (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_Y”</td>
	<td valign="top" >SIGNED_RANGE</td>
	<td valign="top" >Min=INT_MIN, Max=INT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout CRTC (destination) y coordinate (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_W”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout CRTC (destination) width (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_H”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=UINT_MAX</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout CRTC (destination) height (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“FB_ID”</td>
	<td valign="top" >OBJECT</td>
	<td valign="top" >DRM_MODE_OBJECT_FB</td>
	<td valign="top" >Plane</td>
	<td valign="top" >Scanout framebuffer (atomic)</td>
	</tr>
	<tr>
	<td valign="top" >“CRTC_ID”</td>
	<td valign="top" >OBJECT</td>
	<td valign="top" >DRM_MODE_OBJECT_CRTC</td>
	<td valign="top" >Plane</td>
	<td valign="top" >CRTC that plane is attached to (atomic)</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >DVI-I</td>
	<td valign="top" >“subconnector”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ “Unknown”, “DVI-D”, “DVI-A” }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“select subconnector”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ “Automatic”, “DVI-D”, “DVI-A” }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="13" valign="top" >TV</td>
	<td valign="top" >“subconnector”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "Unknown", "Composite", "SVIDEO", "Component", "SCART" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“select subconnector”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "Automatic", "Composite", "SVIDEO", "Component", "SCART" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "NTSC_M", "NTSC_J", "NTSC_443", "PAL_B" } etc.</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“left margin”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“right margin”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“top margin”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“bottom margin”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“brightness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“contrast”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker reduction”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“overscan”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“saturation”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hue”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >Virtual GPU</td>
	<td valign="top" >“suggested X”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffffff</td>
	<td valign="top" >Connector</td>
	<td valign="top" >property to suggest an X offset for a connector</td>
	</tr>
	<tr>
	<td valign="top" >“suggested Y”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffffff</td>
	<td valign="top" >Connector</td>
	<td valign="top" >property to suggest an Y offset for a connector</td>
	</tr>
	<tr>
	<td rowspan="3" valign="top" >Optional</td>
	<td valign="top" >“scaling mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "None", "Full", "Center", "Full aspect" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"aspect ratio"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "None", "4:3", "16:9" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >DRM property to set aspect ratio from user space app.
		This enum is made generic to allow addition of custom aspect
		ratios.</td>
	</tr>
	<tr>
	<td valign="top" >“dirty”</td>
	<td valign="top" >ENUM | IMMUTABLE</td>
	<td valign="top" >{ "Off", "On", "Annotate" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="20" valign="top" >i915</td>
	<td rowspan="2" valign="top" >Generic</td>
	<td valign="top" >"Broadcast RGB"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "Automatic", "Full", "Limited 16:235" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“audio”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "force-dvi", "off", "auto", "on" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="17" valign="top" >SDVO-TV</td>
	<td valign="top" >“mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "NTSC_M", "NTSC_J", "NTSC_443", "PAL_B" } etc.</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"left_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"right_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"top_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"bottom_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hpos”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“vpos”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“contrast”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“saturation”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hue”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“sharpness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter_adaptive”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter_2d”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“tv_chroma_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“tv_luma_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“dot_crawl”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >SDVO-TV/LVDS</td>
	<td valign="top" >“brightness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >CDV gma-500</td>
	<td rowspan="2" valign="top" >Generic</td>
	<td valign="top" >"Broadcast RGB"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ “Full”, “Limited 16:235” }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"Broadcast RGB"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ “off”, “auto”, “on” }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="19" valign="top" >Poulsbo</td>
	<td rowspan="1" valign="top" >Generic</td>
	<td valign="top" >“backlight”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=100</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="17" valign="top" >SDVO-TV</td>
	<td valign="top" >“mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "NTSC_M", "NTSC_J", "NTSC_443", "PAL_B" } etc.</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"left_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"right_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"top_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"bottom_margin"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hpos”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“vpos”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“contrast”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“saturation”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hue”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“sharpness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter_adaptive”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“flicker_filter_2d”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“tv_chroma_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“tv_luma_filter”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“dot_crawl”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >SDVO-TV/LVDS</td>
	<td valign="top" >“brightness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max= SDVO dependent</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="11" valign="top" >armada</td>
	<td rowspan="2" valign="top" >CRTC</td>
	<td valign="top" >"CSC_YUV"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "Auto" , "CCIR601", "CCIR709" }</td>
	<td valign="top" >CRTC</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"CSC_RGB"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "Auto", "Computer system", "Studio" }</td>
	<td valign="top" >CRTC</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="9" valign="top" >Overlay</td>
	<td valign="top" >"colorkey"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey_min"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey_max"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey_val"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey_alpha"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0xffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey_mode"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "disabled", "Y component", "U component"
	, "V component", "RGB", “R component", "G component", "B component" }</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"brightness"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=256 + 255</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"contrast"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0x7fff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"saturation"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0x7fff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >exynos</td>
	<td valign="top" >CRTC</td>
	<td valign="top" >“mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "normal", "blank" }</td>
	<td valign="top" >CRTC</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >Overlay</td>
	<td valign="top" >“zpos”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=MAX_PLANE-1</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >i2c/ch7006_drv</td>
	<td valign="top" >Generic</td>
	<td valign="top" >“scale”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=2</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="1" valign="top" >TV</td>
	<td valign="top" >“mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "PAL", "PAL-M","PAL-N"}, ”PAL-Nc"
	, "PAL-60", "NTSC-M", "NTSC-J" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="15" valign="top" >nouveau</td>
	<td rowspan="6" valign="top" >NV10 Overlay</td>
	<td valign="top" >"colorkey"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0x01ffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“contrast”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=8192-1</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“brightness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1024</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“hue”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=359</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“saturation”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=8192-1</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“iturbt_709”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="2" valign="top" >Nv04 Overlay</td>
	<td valign="top" >“colorkey”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0x01ffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“brightness”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1024</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="7" valign="top" >Display</td>
	<td valign="top" >“dithering mode”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "auto", "off", "on" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“dithering depth”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "auto", "off", "on", "static 2x2", "dynamic 2x2", "temporal" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“underscan”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "auto", "6 bpc", "8 bpc" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“underscan hborder”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=128</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“underscan vborder”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=128</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“vibrant hue”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=180</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >“color vibrance”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=200</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >omap</td>
	<td valign="top" >Generic</td>
	<td valign="top" >“zorder”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=3</td>
	<td valign="top" >CRTC, Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >qxl</td>
	<td valign="top" >Generic</td>
	<td valign="top" >“hotplug_mode_update"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="9" valign="top" >radeon</td>
	<td valign="top" >DVI-I</td>
	<td valign="top" >“coherent”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >DAC enable load detect</td>
	<td valign="top" >“load detection”</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=1</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >TV Standard</td>
	<td valign="top" >"tv standard"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "ntsc", "pal", "pal-m", "pal-60", "ntsc-j"
	, "scart-pal", "pal-cn", "secam" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >legacy TMDS PLL detect</td>
	<td valign="top" >"tmds_pll"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "driver", "bios" }</td>
	<td valign="top" >-</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="3" valign="top" >Underscan</td>
	<td valign="top" >"underscan"</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "off", "on", "auto" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"underscan hborder"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=128</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"underscan vborder"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=128</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >Audio</td>
	<td valign="top" >“audio”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "off", "on", "auto" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >FMT Dithering</td>
	<td valign="top" >“dither”</td>
	<td valign="top" >ENUM</td>
	<td valign="top" >{ "off", "on" }</td>
	<td valign="top" >Connector</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td rowspan="3" valign="top" >rcar-du</td>
	<td rowspan="3" valign="top" >Generic</td>
	<td valign="top" >"alpha"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=255</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"colorkey"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=0, Max=0x01ffffff</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	<tr>
	<td valign="top" >"zpos"</td>
	<td valign="top" >RANGE</td>
	<td valign="top" >Min=1, Max=7</td>
	<td valign="top" >Plane</td>
	<td valign="top" >TBD</td>
	</tr>
	</tbody>
	</table>
    </sect2>
  </sect1>

  <!-- Internals: vertical blanking -->

  <sect1 id="drm-vertical-blank">
    <title>Vertical Blanking</title>
    <para>
      Vertical blanking plays a major role in graphics rendering. To achieve
      tear-free display, users must synchronize page flips and/or rendering to
      vertical blanking. The DRM API offers ioctls to perform page flips
      synchronized to vertical blanking and wait for vertical blanking.
    </para>
    <para>
      The DRM core handles most of the vertical blanking management logic, which
      involves filtering out spurious interrupts, keeping race-free blanking
      counters, coping with counter wrap-around and resets and keeping use
      counts. It relies on the driver to generate vertical blanking interrupts
      and optionally provide a hardware vertical blanking counter. Drivers must
      implement the following operations.
    </para>
    <itemizedlist>
      <listitem>
        <synopsis>int (*enable_vblank) (struct drm_device *dev, int crtc);
void (*disable_vblank) (struct drm_device *dev, int crtc);</synopsis>
        <para>
	  Enable or disable vertical blanking interrupts for the given CRTC.
	</para>
      </listitem>
      <listitem>
        <synopsis>u32 (*get_vblank_counter) (struct drm_device *dev, int crtc);</synopsis>
        <para>
	  Retrieve the value of the vertical blanking counter for the given
	  CRTC. If the hardware maintains a vertical blanking counter its value
	  should be returned. Otherwise drivers can use the
	  <function>drm_vblank_count</function> helper function to handle this
	  operation.
	</para>
      </listitem>
    </itemizedlist>
    <para>
      Drivers must initialize the vertical blanking handling core with a call to
      <function>drm_vblank_init</function> in their
      <methodname>load</methodname> operation. The function will set the struct
      <structname>drm_device</structname>
      <structfield>vblank_disable_allowed</structfield> field to 0. This will
      keep vertical blanking interrupts enabled permanently until the first mode
      set operation, where <structfield>vblank_disable_allowed</structfield> is
      set to 1. The reason behind this is not clear. Drivers can set the field
      to 1 after <function>calling drm_vblank_init</function> to make vertical
      blanking interrupts dynamically managed from the beginning.
    </para>
    <para>
      Vertical blanking interrupts can be enabled by the DRM core or by drivers
      themselves (for instance to handle page flipping operations). The DRM core
      maintains a vertical blanking use count to ensure that the interrupts are
      not disabled while a user still needs them. To increment the use count,
      drivers call <function>drm_vblank_get</function>. Upon return vertical
      blanking interrupts are guaranteed to be enabled.
    </para>
    <para>
      To decrement the use count drivers call
      <function>drm_vblank_put</function>. Only when the use count drops to zero
      will the DRM core disable the vertical blanking interrupts after a delay
      by scheduling a timer. The delay is accessible through the vblankoffdelay
      module parameter or the <varname>drm_vblank_offdelay</varname> global
      variable and expressed in milliseconds. Its default value is 5000 ms.
      Zero means never disable, and a negative value means disable immediately.
      Drivers may override the behaviour by setting the
      <structname>drm_device</structname>
      <structfield>vblank_disable_immediate</structfield> flag, which when set
      causes vblank interrupts to be disabled immediately regardless of the
      drm_vblank_offdelay value. The flag should only be set if there's a
      properly working hardware vblank counter present.
    </para>
    <para>
      When a vertical blanking interrupt occurs drivers only need to call the
      <function>drm_handle_vblank</function> function to account for the
      interrupt.
    </para>
    <para>
      Resources allocated by <function>drm_vblank_init</function> must be freed
      with a call to <function>drm_vblank_cleanup</function> in the driver
      <methodname>unload</methodname> operation handler.
    </para>
    <sect2>
      <title>Vertical Blanking and Interrupt Handling Functions Reference</title>
!Edrivers/gpu/drm/drm_irq.c
!Finclude/drm/drmP.h drm_crtc_vblank_waitqueue
    </sect2>
  </sect1>

  <!-- Internals: open/close, file operations and ioctls -->

  <sect1>
    <title>Open/Close, File Operations and IOCTLs</title>
    <sect2>
      <title>Open and Close</title>
      <synopsis>int (*firstopen) (struct drm_device *);
void (*lastclose) (struct drm_device *);
int (*open) (struct drm_device *, struct drm_file *);
void (*preclose) (struct drm_device *, struct drm_file *);
void (*postclose) (struct drm_device *, struct drm_file *);</synopsis>
      <abstract>Open and close handlers. None of those methods are mandatory.
      </abstract>
      <para>
        The <methodname>firstopen</methodname> method is called by the DRM core
	for legacy UMS (User Mode Setting) drivers only when an application
	opens a device that has no other opened file handle. UMS drivers can
	implement it to acquire device resources. KMS drivers can't use the
	method and must acquire resources in the <methodname>load</methodname>
	method instead.
      </para>
      <para>
	Similarly the <methodname>lastclose</methodname> method is called when
	the last application holding a file handle opened on the device closes
	it, for both UMS and KMS drivers. Additionally, the method is also
	called at module unload time or, for hot-pluggable devices, when the
	device is unplugged. The <methodname>firstopen</methodname> and
	<methodname>lastclose</methodname> calls can thus be unbalanced.
      </para>
      <para>
        The <methodname>open</methodname> method is called every time the device
	is opened by an application. Drivers can allocate per-file private data
	in this method and store them in the struct
	<structname>drm_file</structname> <structfield>driver_priv</structfield>
	field. Note that the <methodname>open</methodname> method is called
	before <methodname>firstopen</methodname>.
      </para>
      <para>
        The close operation is split into <methodname>preclose</methodname> and
	<methodname>postclose</methodname> methods. Drivers must stop and
	cleanup all per-file operations in the <methodname>preclose</methodname>
	method. For instance pending vertical blanking and page flip events must
	be cancelled. No per-file operation is allowed on the file handle after
	returning from the <methodname>preclose</methodname> method.
      </para>
      <para>
        Finally the <methodname>postclose</methodname> method is called as the
	last step of the close operation, right before calling the
	<methodname>lastclose</methodname> method if no other open file handle
	exists for the device. Drivers that have allocated per-file private data
	in the <methodname>open</methodname> method should free it here.
      </para>
      <para>
        The <methodname>lastclose</methodname> method should restore CRTC and
	plane properties to default value, so that a subsequent open of the
	device will not inherit state from the previous user. It can also be
	used to execute delayed power switching state changes, e.g. in
	conjunction with the vga_switcheroo infrastructure (see
	<xref linkend="vga_switcheroo"/>). Beyond that KMS drivers should not
	do any further cleanup. Only legacy UMS drivers might need to clean up
	device state so that the vga console or an independent fbdev driver
	could take over.
      </para>
    </sect2>
    <sect2>
      <title>File Operations</title>
      <synopsis>const struct file_operations *fops</synopsis>
      <abstract>File operations for the DRM device node.</abstract>
      <para>
        Drivers must define the file operations structure that forms the DRM
	userspace API entry point, even though most of those operations are
	implemented in the DRM core. The <methodname>open</methodname>,
	<methodname>release</methodname> and <methodname>ioctl</methodname>
	operations are handled by
	<programlisting>
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
  #ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
  #endif
        </programlisting>
      </para>
      <para>
        Drivers that implement private ioctls that requires 32/64bit
	compatibility support must provide their own
	<methodname>compat_ioctl</methodname> handler that processes private
	ioctls and calls <function>drm_compat_ioctl</function> for core ioctls.
      </para>
      <para>
        The <methodname>read</methodname> and <methodname>poll</methodname>
	operations provide support for reading DRM events and polling them. They
	are implemented by
	<programlisting>
	.poll = drm_poll,
	.read = drm_read,
	.llseek = no_llseek,
	</programlisting>
      </para>
      <para>
        The memory mapping implementation varies depending on how the driver
	manages memory. Pre-GEM drivers will use <function>drm_mmap</function>,
	while GEM-aware drivers will use <function>drm_gem_mmap</function>. See
	<xref linkend="drm-gem"/>.
	<programlisting>
	.mmap = drm_gem_mmap,
	</programlisting>
      </para>
      <para>
        No other file operation is supported by the DRM API.
      </para>
    </sect2>
    <sect2>
      <title>IOCTLs</title>
      <synopsis>struct drm_ioctl_desc *ioctls;
int num_ioctls;</synopsis>
      <abstract>Driver-specific ioctls descriptors table.</abstract>
      <para>
        Driver-specific ioctls numbers start at DRM_COMMAND_BASE. The ioctls
	descriptors table is indexed by the ioctl number offset from the base
	value. Drivers can use the DRM_IOCTL_DEF_DRV() macro to initialize the
	table entries.
      </para>
      <para>
        <programlisting>DRM_IOCTL_DEF_DRV(ioctl, func, flags)</programlisting>
	<para>
	  <parameter>ioctl</parameter> is the ioctl name. Drivers must define
	  the DRM_##ioctl and DRM_IOCTL_##ioctl macros to the ioctl number
	  offset from DRM_COMMAND_BASE and the ioctl number respectively. The
	  first macro is private to the device while the second must be exposed
	  to userspace in a public header.
	</para>
	<para>
	  <parameter>func</parameter> is a pointer to the ioctl handler function
	  compatible with the <type>drm_ioctl_t</type> type.
	  <programlisting>typedef int drm_ioctl_t(struct drm_device *dev, void *data,
		struct drm_file *file_priv);</programlisting>
	</para>
	<para>
	  <parameter>flags</parameter> is a bitmask combination of the following
	  values. It restricts how the ioctl is allowed to be called.
	  <itemizedlist>
	    <listitem><para>
	      DRM_AUTH - Only authenticated callers allowed
	    </para></listitem>
	    <listitem><para>
	      DRM_MASTER - The ioctl can only be called on the master file
	      handle
	    </para></listitem>
            <listitem><para>
	      DRM_ROOT_ONLY - Only callers with the SYSADMIN capability allowed
	    </para></listitem>
            <listitem><para>
	      DRM_CONTROL_ALLOW - The ioctl can only be called on a control
	      device
	    </para></listitem>
            <listitem><para>
	      DRM_UNLOCKED - The ioctl handler will be called without locking
	      the DRM global mutex. This is the enforced default for kms drivers
	      (i.e. using the DRIVER_MODESET flag) and hence shouldn't be used
	      any more for new drivers.
	    </para></listitem>
	  </itemizedlist>
	</para>
      </para>
!Edrivers/gpu/drm/drm_ioctl.c
    </sect2>
  </sect1>
  <sect1>
    <title>Legacy Support Code</title>
    <para>
      The section very briefly covers some of the old legacy support code which
      is only used by old DRM drivers which have done a so-called shadow-attach
      to the underlying device instead of registering as a real driver. This
      also includes some of the old generic buffer management and command
      submission code. Do not use any of this in new and modern drivers.
    </para>

    <sect2>
      <title>Legacy Suspend/Resume</title>
      <para>
	The DRM core provides some suspend/resume code, but drivers wanting full
	suspend/resume support should provide save() and restore() functions.
	These are called at suspend, hibernate, or resume time, and should perform
	any state save or restore required by your device across suspend or
	hibernate states.
      </para>
      <synopsis>int (*suspend) (struct drm_device *, pm_message_t state);
  int (*resume) (struct drm_device *);</synopsis>
      <para>
	Those are legacy suspend and resume methods which
	<emphasis>only</emphasis> work with the legacy shadow-attach driver
	registration functions. New driver should use the power management
	interface provided by their bus type (usually through
	the struct <structname>device_driver</structname> dev_pm_ops) and set
	these methods to NULL.
      </para>
    </sect2>

    <sect2>
      <title>Legacy DMA Services</title>
      <para>
	This should cover how DMA mapping etc. is supported by the core.
	These functions are deprecated and should not be used.
      </para>
    </sect2>
  </sect1>
  </chapter>

<!-- TODO

- Add a glossary
- Document the struct_mutex catch-all lock
- Document connector properties

- Why is the load method optional?
- What are drivers supposed to set the initial display state to, and how?
  Connector's DPMS states are not initialized and are thus equal to
  DRM_MODE_DPMS_ON. The fbcon compatibility layer calls
  drm_helper_disable_unused_functions(), which disables unused encoders and
  CRTCs, but doesn't touch the connectors' DPMS state, and
  drm_helper_connector_dpms() in reaction to fbdev blanking events. Do drivers
  that don't implement (or just don't use) fbcon compatibility need to call
  those functions themselves?
- KMS drivers must call drm_vblank_pre_modeset() and drm_vblank_post_modeset()
  around mode setting. Should this be done in the DRM core?
- vblank_disable_allowed is set to 1 in the first drm_vblank_post_modeset()
  call and never set back to 0. It seems to be safe to permanently set it to 1
  in drm_vblank_init() for KMS driver, and it might be safe for UMS drivers as
  well. This should be investigated.
- crtc and connector .save and .restore operations are only used internally in
  drivers, should they be removed from the core?
- encoder mid-layer .save and .restore operations are only used internally in
  drivers, should they be removed from the core?
- encoder mid-layer .detect operation is only used internally in drivers,
  should it be removed from the core?
-->

  <!-- External interfaces -->

  <chapter id="drmExternals">
    <title>Userland interfaces</title>
    <para>
      The DRM core exports several interfaces to applications,
      generally intended to be used through corresponding libdrm
      wrapper functions.  In addition, drivers export device-specific
      interfaces for use by userspace drivers &amp; device-aware
      applications through ioctls and sysfs files.
    </para>
    <para>
      External interfaces include: memory mapping, context management,
      DMA operations, AGP management, vblank control, fence
      management, memory management, and output management.
    </para>
    <para>
      Cover generic ioctls and sysfs layout here.  We only need high-level
      info, since man pages should cover the rest.
    </para>

  <!-- External: render nodes -->

    <sect1>
      <title>Render nodes</title>
      <para>
        DRM core provides multiple character-devices for user-space to use.
        Depending on which device is opened, user-space can perform a different
        set of operations (mainly ioctls). The primary node is always created
        and called card&lt;num&gt;. Additionally, a currently
        unused control node, called controlD&lt;num&gt; is also
        created. The primary node provides all legacy operations and
        historically was the only interface used by userspace. With KMS, the
        control node was introduced. However, the planned KMS control interface
        has never been written and so the control node stays unused to date.
      </para>
      <para>
        With the increased use of offscreen renderers and GPGPU applications,
        clients no longer require running compositors or graphics servers to
        make use of a GPU. But the DRM API required unprivileged clients to
        authenticate to a DRM-Master prior to getting GPU access. To avoid this
        step and to grant clients GPU access without authenticating, render
        nodes were introduced. Render nodes solely serve render clients, that
        is, no modesetting or privileged ioctls can be issued on render nodes.
        Only non-global rendering commands are allowed. If a driver supports
        render nodes, it must advertise it via the DRIVER_RENDER
        DRM driver capability. If not supported, the primary node must be used
        for render clients together with the legacy drmAuth authentication
        procedure.
      </para>
      <para>
        If a driver advertises render node support, DRM core will create a
        separate render node called renderD&lt;num&gt;. There will
        be one render node per device. No ioctls except  PRIME-related ioctls
        will be allowed on this node. Especially GEM_OPEN will be
        explicitly prohibited. Render nodes are designed to avoid the
        buffer-leaks, which occur if clients guess the flink names or mmap
        offsets on the legacy interface. Additionally to this basic interface,
        drivers must mark their driver-dependent render-only ioctls as
        DRM_RENDER_ALLOW so render clients can use them. Driver
        authors must be careful not to allow any privileged ioctls on render
        nodes.
      </para>
      <para>
        With render nodes, user-space can now control access to the render node
        via basic file-system access-modes. A running graphics server which
        authenticates clients on the privileged primary/legacy node is no longer
        required. Instead, a client can open the render node and is immediately
        granted GPU access. Communication between clients (or servers) is done
        via PRIME. FLINK from render node to legacy node is not supported. New
        clients must not use the insecure FLINK interface.
      </para>
      <para>
        Besides dropping all modeset/global ioctls, render nodes also drop the
        DRM-Master concept. There is no reason to associate render clients with
        a DRM-Master as they are independent of any graphics server. Besides,
        they must work without any running master, anyway.
        Drivers must be able to run without a master object if they support
        render nodes. If, on the other hand, a driver requires shared state
        between clients which is visible to user-space and accessible beyond
        open-file boundaries, they cannot support render nodes.
      </para>
    </sect1>

  <!-- External: vblank handling -->

    <sect1>
      <title>VBlank event handling</title>
      <para>
        The DRM core exposes two vertical blank related ioctls:
        <variablelist>
          <varlistentry>
            <term>DRM_IOCTL_WAIT_VBLANK</term>
            <listitem>
              <para>
                This takes a struct drm_wait_vblank structure as its argument,
                and it is used to block or request a signal when a specified
                vblank event occurs.
              </para>
            </listitem>
          </varlistentry>
          <varlistentry>
            <term>DRM_IOCTL_MODESET_CTL</term>
            <listitem>
              <para>
		This was only used for user-mode-settind drivers around
		modesetting changes to allow the kernel to update the vblank
		interrupt after mode setting, since on many devices the vertical
		blank counter is reset to 0 at some point during modeset. Modern
		drivers should not call this any more since with kernel mode
		setting it is a no-op.
              </para>
            </listitem>
          </varlistentry>
        </variablelist>
      </para>
    </sect1>

  </chapter>
</part>
<part id="drmDrivers">
  <title>DRM Drivers</title>

  <partintro>
    <para>
      This second part of the GPU Driver Developer's Guide documents driver
      code, implementation details and also all the driver-specific userspace
      interfaces. Especially since all hardware-acceleration interfaces to
      userspace are driver specific for efficiency and other reasons these
      interfaces can be rather substantial. Hence every driver has its own
      chapter.
    </para>
  </partintro>

  <chapter id="drmI915">
    <title>drm/i915 Intel GFX Driver</title>
    <para>
      The drm/i915 driver supports all (with the exception of some very early
      models) integrated GFX chipsets with both Intel display and rendering
      blocks. This excludes a set of SoC platforms with an SGX rendering unit,
      those have basic support through the gma500 drm driver.
    </para>
    <sect1>
      <title>Core Driver Infrastructure</title>
      <para>
	This section covers core driver infrastructure used by both the display
	and the GEM parts of the driver.
      </para>
      <sect2>
        <title>Runtime Power Management</title>
!Pdrivers/gpu/drm/i915/intel_runtime_pm.c runtime pm
!Idrivers/gpu/drm/i915/intel_runtime_pm.c
!Idrivers/gpu/drm/i915/intel_uncore.c
      </sect2>
      <sect2>
        <title>Interrupt Handling</title>
!Pdrivers/gpu/drm/i915/i915_irq.c interrupt handling
!Fdrivers/gpu/drm/i915/i915_irq.c intel_irq_init intel_irq_init_hw intel_hpd_init
!Fdrivers/gpu/drm/i915/i915_irq.c intel_runtime_pm_disable_interrupts
!Fdrivers/gpu/drm/i915/i915_irq.c intel_runtime_pm_enable_interrupts
      </sect2>
      <sect2>
        <title>Intel GVT-g Guest Support(vGPU)</title>
!Pdrivers/gpu/drm/i915/i915_vgpu.c Intel GVT-g guest support
!Idrivers/gpu/drm/i915/i915_vgpu.c
      </sect2>
    </sect1>
    <sect1>
      <title>Display Hardware Handling</title>
      <para>
        This section covers everything related to the display hardware including
        the mode setting infrastructure, plane, sprite and cursor handling and
        display, output probing and related topics.
      </para>
      <sect2>
        <title>Mode Setting Infrastructure</title>
        <para>
          The i915 driver is thus far the only DRM driver which doesn't use the
          common DRM helper code to implement mode setting sequences. Thus it
          has its own tailor-made infrastructure for executing a display
          configuration change.
        </para>
      </sect2>
      <sect2>
        <title>Frontbuffer Tracking</title>
!Pdrivers/gpu/drm/i915/intel_frontbuffer.c frontbuffer tracking
!Idrivers/gpu/drm/i915/intel_frontbuffer.c
!Fdrivers/gpu/drm/i915/i915_gem.c i915_gem_track_fb
      </sect2>
      <sect2>
        <title>Display FIFO Underrun Reporting</title>
!Pdrivers/gpu/drm/i915/intel_fifo_underrun.c fifo underrun handling
!Idrivers/gpu/drm/i915/intel_fifo_underrun.c
      </sect2>
      <sect2>
        <title>Plane Configuration</title>
        <para>
	  This section covers plane configuration and composition with the
	  primary plane, sprites, cursors and overlays. This includes the
	  infrastructure to do atomic vsync'ed updates of all this state and
	  also tightly coupled topics like watermark setup and computation,
	  framebuffer compression and panel self refresh.
        </para>
      </sect2>
      <sect2>
        <title>Atomic Plane Helpers</title>
!Pdrivers/gpu/drm/i915/intel_atomic_plane.c atomic plane helpers
!Idrivers/gpu/drm/i915/intel_atomic_plane.c
      </sect2>
      <sect2>
        <title>Output Probing</title>
        <para>
	  This section covers output probing and related infrastructure like the
	  hotplug interrupt storm detection and mitigation code. Note that the
	  i915 driver still uses most of the common DRM helper code for output
	  probing, so those sections fully apply.
        </para>
      </sect2>
      <sect2>
        <title>Hotplug</title>
!Pdrivers/gpu/drm/i915/intel_hotplug.c Hotplug
!Idrivers/gpu/drm/i915/intel_hotplug.c
      </sect2>
      <sect2>
	<title>High Definition Audio</title>
!Pdrivers/gpu/drm/i915/intel_audio.c High Definition Audio over HDMI and Display Port
!Idrivers/gpu/drm/i915/intel_audio.c
!Iinclude/drm/i915_component.h
      </sect2>
      <sect2>
	<title>Panel Self Refresh PSR (PSR/SRD)</title>
!Pdrivers/gpu/drm/i915/intel_psr.c Panel Self Refresh (PSR/SRD)
!Idrivers/gpu/drm/i915/intel_psr.c
      </sect2>
      <sect2>
	<title>Frame Buffer Compression (FBC)</title>
!Pdrivers/gpu/drm/i915/intel_fbc.c Frame Buffer Compression (FBC)
!Idrivers/gpu/drm/i915/intel_fbc.c
      </sect2>
      <sect2>
        <title>Display Refresh Rate Switching (DRRS)</title>
!Pdrivers/gpu/drm/i915/intel_dp.c Display Refresh Rate Switching (DRRS)
!Fdrivers/gpu/drm/i915/intel_dp.c intel_dp_set_drrs_state
!Fdrivers/gpu/drm/i915/intel_dp.c intel_edp_drrs_enable
!Fdrivers/gpu/drm/i915/intel_dp.c intel_edp_drrs_disable
!Fdrivers/gpu/drm/i915/intel_dp.c intel_edp_drrs_invalidate
!Fdrivers/gpu/drm/i915/intel_dp.c intel_edp_drrs_flush
!Fdrivers/gpu/drm/i915/intel_dp.c intel_dp_drrs_init

      </sect2>
      <sect2>
        <title>DPIO</title>
!Pdrivers/gpu/drm/i915/i915_reg.h DPIO
	<table id="dpiox2">
	  <title>Dual channel PHY (VLV/CHV/BXT)</title>
	  <tgroup cols="8">
	    <colspec colname="c0" />
	    <colspec colname="c1" />
	    <colspec colname="c2" />
	    <colspec colname="c3" />
	    <colspec colname="c4" />
	    <colspec colname="c5" />
	    <colspec colname="c6" />
	    <colspec colname="c7" />
	    <spanspec spanname="ch0" namest="c0" nameend="c3" />
	    <spanspec spanname="ch1" namest="c4" nameend="c7" />
	    <spanspec spanname="ch0pcs01" namest="c0" nameend="c1" />
	    <spanspec spanname="ch0pcs23" namest="c2" nameend="c3" />
	    <spanspec spanname="ch1pcs01" namest="c4" nameend="c5" />
	    <spanspec spanname="ch1pcs23" namest="c6" nameend="c7" />
	    <thead>
	      <row>
		<entry spanname="ch0">CH0</entry>
		<entry spanname="ch1">CH1</entry>
	      </row>
	    </thead>
	    <tbody valign="top" align="center">
	      <row>
		<entry spanname="ch0">CMN/PLL/REF</entry>
		<entry spanname="ch1">CMN/PLL/REF</entry>
	      </row>
	      <row>
		<entry spanname="ch0pcs01">PCS01</entry>
		<entry spanname="ch0pcs23">PCS23</entry>
		<entry spanname="ch1pcs01">PCS01</entry>
		<entry spanname="ch1pcs23">PCS23</entry>
	      </row>
	      <row>
		<entry>TX0</entry>
		<entry>TX1</entry>
		<entry>TX2</entry>
		<entry>TX3</entry>
		<entry>TX0</entry>
		<entry>TX1</entry>
		<entry>TX2</entry>
		<entry>TX3</entry>
	      </row>
	      <row>
		<entry spanname="ch0">DDI0</entry>
		<entry spanname="ch1">DDI1</entry>
	      </row>
	    </tbody>
	  </tgroup>
	</table>
	<table id="dpiox1">
	  <title>Single channel PHY (CHV/BXT)</title>
	  <tgroup cols="4">
	    <colspec colname="c0" />
	    <colspec colname="c1" />
	    <colspec colname="c2" />
	    <colspec colname="c3" />
	    <spanspec spanname="ch0" namest="c0" nameend="c3" />
	    <spanspec spanname="ch0pcs01" namest="c0" nameend="c1" />
	    <spanspec spanname="ch0pcs23" namest="c2" nameend="c3" />
	    <thead>
	      <row>
		<entry spanname="ch0">CH0</entry>
	      </row>
	    </thead>
	    <tbody valign="top" align="center">
	      <row>
		<entry spanname="ch0">CMN/PLL/REF</entry>
	      </row>
	      <row>
		<entry spanname="ch0pcs01">PCS01</entry>
		<entry spanname="ch0pcs23">PCS23</entry>
	      </row>
	      <row>
		<entry>TX0</entry>
		<entry>TX1</entry>
		<entry>TX2</entry>
		<entry>TX3</entry>
	      </row>
	      <row>
		<entry spanname="ch0">DDI2</entry>
	      </row>
	    </tbody>
	  </tgroup>
	</table>
      </sect2>

      <sect2>
       <title>CSR firmware support for DMC</title>
!Pdrivers/gpu/drm/i915/intel_csr.c csr support for dmc
!Idrivers/gpu/drm/i915/intel_csr.c
      </sect2>
    </sect1>

    <sect1>
      <title>Memory Management and Command Submission</title>
      <para>
	This sections covers all things related to the GEM implementation in the
	i915 driver.
      </para>
      <sect2>
        <title>Batchbuffer Parsing</title>
!Pdrivers/gpu/drm/i915/i915_cmd_parser.c batch buffer command parser
!Idrivers/gpu/drm/i915/i915_cmd_parser.c
      </sect2>
      <sect2>
        <title>Batchbuffer Pools</title>
!Pdrivers/gpu/drm/i915/i915_gem_batch_pool.c batch pool
!Idrivers/gpu/drm/i915/i915_gem_batch_pool.c
      </sect2>
      <sect2>
        <title>Logical Rings, Logical Ring Contexts and Execlists</title>
!Pdrivers/gpu/drm/i915/intel_lrc.c Logical Rings, Logical Ring Contexts and Execlists
!Idrivers/gpu/drm/i915/intel_lrc.c
      </sect2>
      <sect2>
        <title>Global GTT views</title>
!Pdrivers/gpu/drm/i915/i915_gem_gtt.c Global GTT views
!Idrivers/gpu/drm/i915/i915_gem_gtt.c
      </sect2>
      <sect2>
        <title>GTT Fences and Swizzling</title>
!Idrivers/gpu/drm/i915/i915_gem_fence.c
        <sect3>
          <title>Global GTT Fence Handling</title>
!Pdrivers/gpu/drm/i915/i915_gem_fence.c fence register handling
        </sect3>
        <sect3>
          <title>Hardware Tiling and Swizzling Details</title>
!Pdrivers/gpu/drm/i915/i915_gem_fence.c tiling swizzling details
        </sect3>
      </sect2>
      <sect2>
        <title>Object Tiling IOCTLs</title>
!Idrivers/gpu/drm/i915/i915_gem_tiling.c
!Pdrivers/gpu/drm/i915/i915_gem_tiling.c buffer object tiling
      </sect2>
      <sect2>
        <title>Buffer Object Eviction</title>
	<para>
	  This section documents the interface functions for evicting buffer
	  objects to make space available in the virtual gpu address spaces.
	  Note that this is mostly orthogonal to shrinking buffer objects
	  caches, which has the goal to make main memory (shared with the gpu
	  through the unified memory architecture) available.
	</para>
!Idrivers/gpu/drm/i915/i915_gem_evict.c
      </sect2>
      <sect2>
        <title>Buffer Object Memory Shrinking</title>
	<para>
	  This section documents the interface function for shrinking memory
	  usage of buffer object caches. Shrinking is used to make main memory
	  available.  Note that this is mostly orthogonal to evicting buffer
	  objects, which has the goal to make space in gpu virtual address
	  spaces.
	</para>
!Idrivers/gpu/drm/i915/i915_gem_shrinker.c
      </sect2>
    </sect1>
    <sect1>
      <title>GuC-based Command Submission</title>
      <sect2>
        <title>GuC</title>
!Pdrivers/gpu/drm/i915/intel_guc_loader.c GuC-specific firmware loader
!Idrivers/gpu/drm/i915/intel_guc_loader.c
      </sect2>
      <sect2>
        <title>GuC Client</title>
!Pdrivers/gpu/drm/i915/i915_guc_submission.c GuC-based command submissison
!Idrivers/gpu/drm/i915/i915_guc_submission.c
      </sect2>
    </sect1>

    <sect1>
      <title> Tracing </title>
      <para>
    This sections covers all things related to the tracepoints implemented in
    the i915 driver.
      </para>
      <sect2>
        <title> i915_ppgtt_create and i915_ppgtt_release </title>
!Pdrivers/gpu/drm/i915/i915_trace.h i915_ppgtt_create and i915_ppgtt_release tracepoints
      </sect2>
      <sect2>
        <title> i915_context_create and i915_context_free </title>
!Pdrivers/gpu/drm/i915/i915_trace.h i915_context_create and i915_context_free tracepoints
      </sect2>
      <sect2>
        <title> switch_mm </title>
!Pdrivers/gpu/drm/i915/i915_trace.h switch_mm tracepoint
      </sect2>
    </sect1>

  </chapter>
!Cdrivers/gpu/drm/i915/i915_irq.c
</part>

<part id="vga_switcheroo">
  <title>vga_switcheroo</title>
  <partintro>
!Pdrivers/gpu/vga/vga_switcheroo.c Overview
  </partintro>

  <chapter id="modes_of_use">
    <title>Modes of Use</title>
  <sect1>
    <title>Manual switching and manual power control</title>
!Pdrivers/gpu/vga/vga_switcheroo.c Manual switching and manual power control
  </sect1>
  <sect1>
    <title>Driver power control</title>
!Pdrivers/gpu/vga/vga_switcheroo.c Driver power control
  </sect1>
  </chapter>

  <chapter id="pubfunctions">
    <title>Public functions</title>
!Edrivers/gpu/vga/vga_switcheroo.c
  </chapter>

  <chapter id="pubstructures">
    <title>Public structures</title>
!Finclude/linux/vga_switcheroo.h vga_switcheroo_handler
!Finclude/linux/vga_switcheroo.h vga_switcheroo_client_ops
  </chapter>

  <chapter id="pubconstants">
    <title>Public constants</title>
!Finclude/linux/vga_switcheroo.h vga_switcheroo_client_id
!Finclude/linux/vga_switcheroo.h vga_switcheroo_state
  </chapter>

  <chapter id="privstructures">
    <title>Private structures</title>
!Fdrivers/gpu/vga/vga_switcheroo.c vgasr_priv
!Fdrivers/gpu/vga/vga_switcheroo.c vga_switcheroo_client
  </chapter>

!Cdrivers/gpu/vga/vga_switcheroo.c
!Cinclude/linux/vga_switcheroo.h
</part>

</book>
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         <section id="frontend-properties">
<title>DVB Frontend properties</title>
<para>Tuning into a Digital TV physical channel and starting decoding it
    requires changing a set of parameters, in order to control the
    tuner, the demodulator, the Linear Low-noise Amplifier (LNA) and to set the
    antenna subsystem via Satellite Equipment Control (SEC), on satellite
    systems. The actual parameters are specific to each particular digital
    TV standards, and may change as the digital TV specs evolves.</para>
<para>In the past, the strategy used was to have a union with the parameters
    needed to tune for DVB-S, DVB-C, DVB-T and ATSC delivery systems grouped
    there. The problem is that, as the second generation standards appeared,
    those structs were not big enough to contain the additional parameters.
    Also, the union didn't have any space left to be expanded without breaking
    userspace. So, the decision was to deprecate the legacy union/struct based
    approach, in favor of a properties set approach.</para>

<para>NOTE: on Linux DVB API version 3, setting a frontend were done via
    <link linkend="dvb-frontend-parameters">struct  <constant>dvb_frontend_parameters</constant></link>.
    This got replaced on version 5 (also called "S2API", as this API were
    added originally_enabled to provide support for DVB-S2), because the old
    API has a very limited support to new standards and new hardware. This
    section describes the new and recommended way to set the frontend, with
    suppports all digital TV delivery systems.</para>

<para>Example: with the properties based approach, in order to set the tuner
    to a DVB-C channel at 651 kHz, modulated with 256-QAM, FEC 3/4 and symbol
    rate of 5.217 Mbauds, those properties should be sent to
    <link linkend="FE_GET_PROPERTY"><constant>FE_SET_PROPERTY</constant></link> ioctl:</para>
    <itemizedlist>
	<listitem><para>&DTV-DELIVERY-SYSTEM; = SYS_DVBC_ANNEX_A</para></listitem>
	<listitem><para>&DTV-FREQUENCY; = 651000000</para></listitem>
	<listitem><para>&DTV-MODULATION; = QAM_256</para></listitem>
	<listitem><para>&DTV-INVERSION; = INVERSION_AUTO</para></listitem>
	<listitem><para>&DTV-SYMBOL-RATE; = 5217000</para></listitem>
	<listitem><para>&DTV-INNER-FEC; = FEC_3_4</para></listitem>
	<listitem><para>&DTV-TUNE;</para></listitem>
    </itemizedlist>

<para>The code that would do the above is:</para>
<programlisting>
#include &lt;stdio.h&gt;
#include &lt;fcntl.h&gt;
#include &lt;sys/ioctl.h&gt;
#include &lt;linux/dvb/frontend.h&gt;

static struct dtv_property props[] = {
	{ .cmd = DTV_DELIVERY_SYSTEM, .u.data = SYS_DVBC_ANNEX_A },
	{ .cmd = DTV_FREQUENCY,       .u.data = 651000000 },
	{ .cmd = DTV_MODULATION,      .u.data = QAM_256 },
	{ .cmd = DTV_INVERSION,       .u.data = INVERSION_AUTO },
	{ .cmd = DTV_SYMBOL_RATE,     .u.data = 5217000 },
	{ .cmd = DTV_INNER_FEC,       .u.data = FEC_3_4 },
	{ .cmd = DTV_TUNE }
};

static struct dtv_properties dtv_prop = {
	.num = 6, .props = props
};

int main(void)
{
	int fd = open("/dev/dvb/adapter0/frontend0", O_RDWR);

	if (!fd) {
	    perror ("open");
	    return -1;
	}
	if (ioctl(fd, FE_SET_PROPERTY, &amp;dtv_prop) == -1) {
		perror("ioctl");
		return -1;
	}
	printf("Frontend set\n");
	return 0;
}
</programlisting>

<para>NOTE: While it is possible to directly call the Kernel code like the
    above example, it is strongly recommended to use
    <ulink url="http://linuxtv.org/docs/libdvbv5/index.html">libdvbv5</ulink>,
    as it provides abstraction to work with the supported digital TV standards
    and provides methods for usual operations like program scanning and to
    read/write channel descriptor files.</para>

<section id="dtv-stats">
<title>struct <structname>dtv_stats</structname></title>
<programlisting>
struct dtv_stats {
	__u8 scale;	/* enum fecap_scale_params type */
	union {
		__u64 uvalue;	/* for counters and relative scales */
		__s64 svalue;	/* for 1/1000 dB measures */
	};
} __packed;
</programlisting>
</section>
<section id="dtv-fe-stats">
<title>struct <structname>dtv_fe_stats</structname></title>
<programlisting>
#define MAX_DTV_STATS   4

struct dtv_fe_stats {
	__u8 len;
	&dtv-stats; stat[MAX_DTV_STATS];
} __packed;
</programlisting>
</section>

<section id="dtv-property">
<title>struct <structname>dtv_property</structname></title>
<programlisting>
/* Reserved fields should be set to 0 */

struct dtv_property {
	__u32 cmd;
	__u32 reserved[3];
	union {
		__u32 data;
		&dtv-fe-stats; st;
		struct {
			__u8 data[32];
			__u32 len;
			__u32 reserved1[3];
			void *reserved2;
		} buffer;
	} u;
	int result;
} __attribute__ ((packed));

/* num of properties cannot exceed DTV_IOCTL_MAX_MSGS per ioctl */
#define DTV_IOCTL_MAX_MSGS 64
</programlisting>
</section>
<section id="dtv-properties">
<title>struct <structname>dtv_properties</structname></title>
<programlisting>
struct dtv_properties {
	__u32 num;
	&dtv-property; *props;
};
</programlisting>
</section>

<section>
	<title>Property types</title>
<para>
On <link linkend="FE_GET_PROPERTY">FE_GET_PROPERTY and FE_SET_PROPERTY</link>,
the actual action is determined by the dtv_property cmd/data pairs. With one single ioctl, is possible to
get/set up to 64 properties. The actual meaning of each property is described on the next sections.
</para>

<para>The available frontend property types are shown on the next section.</para>
</section>

<section id="fe_property_parameters">
	<title>Digital TV property parameters</title>
	<section id="DTV-UNDEFINED">
	<title><constant>DTV_UNDEFINED</constant></title>
	<para>Used internally. A GET/SET operation for it won't change or return anything.</para>
	</section>
	<section id="DTV-TUNE">
	<title><constant>DTV_TUNE</constant></title>
	<para>Interpret the cache of data, build either a traditional frontend tunerequest so we can pass validation in the <constant>FE_SET_FRONTEND</constant> ioctl.</para>
	</section>
	<section id="DTV-CLEAR">
	<title><constant>DTV_CLEAR</constant></title>
	<para>Reset a cache of data specific to the frontend here. This does not effect hardware.</para>
	</section>
	<section id="DTV-FREQUENCY">
		<title><constant>DTV_FREQUENCY</constant></title>

		<para>Central frequency of the channel.</para>

		<para>Notes:</para>
		<para>1)For satellite delivery systems, it is measured in kHz.
			For the other ones, it is measured in Hz.</para>
		<para>2)For ISDB-T, the channels are usually transmitted with an offset of 143kHz.
			E.g. a valid frequency could be 474143 kHz. The stepping is bound to the bandwidth of
			the channel which is 6MHz.</para>

		<para>3)As in ISDB-Tsb the channel consists of only one or three segments the
			frequency step is 429kHz, 3*429 respectively. As for ISDB-T the
			central frequency of the channel is expected.</para>
	</section>
	<section id="DTV-MODULATION">
	<title><constant>DTV_MODULATION</constant></title>
<para>Specifies the frontend modulation type for delivery systems that supports
    more than one modulation type. The modulation can be one of the types
    defined by &fe-modulation;.</para>


<section id="fe-modulation-t">
<title>Modulation property</title>

<para>Most of the digital TV standards currently offers more than one possible
    modulation (sometimes called as "constellation" on some standards). This
    enum contains the values used by the Kernel. Please note that not all
    modulations are supported by a given standard.</para>

<table pgwide="1" frame="none" id="fe-modulation">
    <title>enum fe_modulation</title>
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
	    <entry id="QPSK"><constant>QPSK</constant></entry>
	    <entry>QPSK modulation</entry>
	</row><row>
	    <entry id="QAM-16"><constant>QAM_16</constant></entry>
	    <entry>16-QAM modulation</entry>
	</row><row>
	    <entry id="QAM-32"><constant>QAM_32</constant></entry>
	    <entry>32-QAM modulation</entry>
	</row><row>
	    <entry id="QAM-64"><constant>QAM_64</constant></entry>
	    <entry>64-QAM modulation</entry>
	</row><row>
	    <entry id="QAM-128"><constant>QAM_128</constant></entry>
	    <entry>128-QAM modulation</entry>
	</row><row>
	    <entry id="QAM-256"><constant>QAM_256</constant></entry>
	    <entry>256-QAM modulation</entry>
	</row><row>
	    <entry id="QAM-AUTO"><constant>QAM_AUTO</constant></entry>
	    <entry>Autodetect QAM modulation</entry>
	</row><row>
	    <entry id="VSB-8"><constant>VSB_8</constant></entry>
	    <entry>8-VSB modulation</entry>
	</row><row>
	    <entry id="VSB-16"><constant>VSB_16</constant></entry>
	    <entry>16-VSB modulation</entry>
	</row><row>
	    <entry id="PSK-8"><constant>PSK_8</constant></entry>
	    <entry>8-PSK modulation</entry>
	</row><row>
	    <entry id="APSK-16"><constant>APSK_16</constant></entry>
	    <entry>16-APSK modulation</entry>
	</row><row>
	    <entry id="APSK-32"><constant>APSK_32</constant></entry>
	    <entry>32-APSK modulation</entry>
	</row><row>
	    <entry id="DQPSK"><constant>DQPSK</constant></entry>
	    <entry>DQPSK modulation</entry>
	</row><row>
	    <entry id="QAM-4-NR"><constant>QAM_4_NR</constant></entry>
	    <entry>4-QAM-NR modulation</entry>
	</row>
        </tbody>
    </tgroup>
</table>
</section>

	</section>
	<section id="DTV-BANDWIDTH-HZ">
		<title><constant>DTV_BANDWIDTH_HZ</constant></title>

		<para>Bandwidth for the channel, in HZ.</para>

		<para>Possible values:
			<constant>1712000</constant>,
			<constant>5000000</constant>,
			<constant>6000000</constant>,
			<constant>7000000</constant>,
			<constant>8000000</constant>,
			<constant>10000000</constant>.
		</para>

		<para>Notes:</para>

		<para>1) For ISDB-T it should be always 6000000Hz (6MHz)</para>
		<para>2) For ISDB-Tsb it can vary depending on the number of connected segments</para>
		<para>3) Bandwidth doesn't apply for DVB-C transmissions, as the bandwidth
			 for DVB-C depends on the symbol rate</para>
		<para>4) Bandwidth in ISDB-T is fixed (6MHz) or can be easily derived from
			other parameters (DTV_ISDBT_SB_SEGMENT_IDX,
			DTV_ISDBT_SB_SEGMENT_COUNT).</para>
		<para>5) DVB-T supports 6, 7 and 8MHz.</para>
		<para>6) In addition, DVB-T2 supports 1.172, 5 and 10MHz.</para>
	</section>
	<section id="DTV-INVERSION">
	<title><constant>DTV_INVERSION</constant></title>

	<para>Specifies if the frontend should do spectral inversion or not.</para>

<section id="fe-spectral-inversion-t">
<title>enum fe_modulation: Frontend spectral inversion</title>

<para>This parameter indicates if spectral inversion should be presumed or not.
    In the automatic setting (<constant>INVERSION_AUTO</constant>) the hardware
    will try to figure out the correct setting by itself. If the hardware
    doesn't support, the DVB core will try to lock at the carrier first with
    inversion off. If it fails, it will try to enable inversion.
</para>

<table pgwide="1" frame="none" id="fe-spectral-inversion">
    <title>enum fe_modulation</title>
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
	    <entry id="INVERSION-OFF"><constant>INVERSION_OFF</constant></entry>
	    <entry>Don't do spectral band inversion.</entry>
	</row><row>
	    <entry id="INVERSION-ON"><constant>INVERSION_ON</constant></entry>
	    <entry>Do spectral band inversion.</entry>
	</row><row>
	    <entry id="INVERSION-AUTO"><constant>INVERSION_AUTO</constant></entry>
	    <entry>Autodetect spectral band inversion.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
</section>

	</section>
	<section id="DTV-DISEQC-MASTER">
	<title><constant>DTV_DISEQC_MASTER</constant></title>
	<para>Currently not implemented.</para>
	</section>
	<section id="DTV-SYMBOL-RATE">
	<title><constant>DTV_SYMBOL_RATE</constant></title>
	<para>Digital TV symbol rate, in bauds (symbols/second). Used on cable standards.</para>
	</section>
	<section id="DTV-INNER-FEC">
	<title><constant>DTV_INNER_FEC</constant></title>
	<para>Used cable/satellite transmissions. The acceptable values are:
	</para>
<section id="fe-code-rate-t">
<title>enum fe_code_rate: type of the Forward Error Correction.</title>

<table pgwide="1" frame="none" id="fe-code-rate">
    <title>enum fe_code_rate</title>
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
	    <entry id="FEC-NONE"><constant>FEC_NONE</constant></entry>
	    <entry>No Forward Error Correction Code</entry>
	</row><row>
	    <entry id="FEC-AUTO"><constant>FEC_AUTO</constant></entry>
	    <entry>Autodetect Error Correction Code</entry>
	</row><row>
	    <entry id="FEC-1-2"><constant>FEC_1_2</constant></entry>
	    <entry>Forward Error Correction Code 1/2</entry>
	</row><row>
	    <entry id="FEC-2-3"><constant>FEC_2_3</constant></entry>
	    <entry>Forward Error Correction Code 2/3</entry>
	</row><row>
	    <entry id="FEC-3-4"><constant>FEC_3_4</constant></entry>
	    <entry>Forward Error Correction Code 3/4</entry>
	</row><row>
	    <entry id="FEC-4-5"><constant>FEC_4_5</constant></entry>
	    <entry>Forward Error Correction Code 4/5</entry>
	</row><row>
	    <entry id="FEC-5-6"><constant>FEC_5_6</constant></entry>
	    <entry>Forward Error Correction Code 5/6</entry>
	</row><row>
	    <entry id="FEC-6-7"><constant>FEC_6_7</constant></entry>
	    <entry>Forward Error Correction Code 6/7</entry>
	</row><row>
	    <entry id="FEC-7-8"><constant>FEC_7_8</constant></entry>
	    <entry>Forward Error Correction Code 7/8</entry>
	</row><row>
	    <entry id="FEC-8-9"><constant>FEC_8_9</constant></entry>
	    <entry>Forward Error Correction Code 8/9</entry>
	</row><row>
	    <entry id="FEC-9-10"><constant>FEC_9_10</constant></entry>
	    <entry>Forward Error Correction Code 9/10</entry>
	</row><row>
	    <entry id="FEC-2-5"><constant>FEC_2_5</constant></entry>
	    <entry>Forward Error Correction Code 2/5</entry>
	</row><row>
	    <entry id="FEC-3-5"><constant>FEC_3_5</constant></entry>
	    <entry>Forward Error Correction Code 3/5</entry>
	</row>
        </tbody>
    </tgroup>
</table>
</section>
	</section>
	<section id="DTV-VOLTAGE">
	<title><constant>DTV_VOLTAGE</constant></title>
	<para>The voltage is usually used with non-DiSEqC capable LNBs to switch
	the polarzation (horizontal/vertical). When using DiSEqC epuipment this
	voltage has to be switched consistently to the DiSEqC commands as
	described in the DiSEqC spec.</para>

<table pgwide="1" frame="none" id="fe-sec-voltage">
    <title id="fe-sec-voltage-t">enum fe_sec_voltage</title>
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
	    <entry align="char" id="SEC-VOLTAGE-13"><constant>SEC_VOLTAGE_13</constant></entry>
	    <entry align="char">Set DC voltage level to 13V</entry>
	</row><row>
	    <entry align="char" id="SEC-VOLTAGE-18"><constant>SEC_VOLTAGE_18</constant></entry>
	    <entry align="char">Set DC voltage level to 18V</entry>
	</row><row>
	    <entry align="char" id="SEC-VOLTAGE-OFF"><constant>SEC_VOLTAGE_OFF</constant></entry>
	    <entry align="char">Don't send any voltage to the antenna</entry>
	</row>
        </tbody>
    </tgroup>
</table>
	</section>
	<section id="DTV-TONE">
	<title><constant>DTV_TONE</constant></title>
	<para>Currently not used.</para>
	</section>
	<section id="DTV-PILOT">
	<title><constant>DTV_PILOT</constant></title>
	<para>Sets DVB-S2 pilot</para>
	<section id="fe-pilot-t">
		<title>fe_pilot type</title>
<table pgwide="1" frame="none" id="fe-pilot">
    <title>enum fe_pilot</title>
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
	    <entry align="char" id="PILOT-ON"><constant>PILOT_ON</constant></entry>
	    <entry align="char">Pilot tones enabled</entry>
	</row><row>
	    <entry align="char" id="PILOT-OFF"><constant>PILOT_OFF</constant></entry>
	    <entry align="char">Pilot tones disabled</entry>
	</row><row>
	    <entry align="char" id="PILOT-AUTO"><constant>PILOT_AUTO</constant></entry>
	    <entry align="char">Autodetect pilot tones</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
	</section>
	<section id="DTV-ROLLOFF">
	<title><constant>DTV_ROLLOFF</constant></title>
		<para>Sets DVB-S2 rolloff</para>

	<section id="fe-rolloff-t">
		<title>fe_rolloff type</title>
<table pgwide="1" frame="none" id="fe-rolloff">
    <title>enum fe_rolloff</title>
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
	    <entry align="char" id="ROLLOFF-35"><constant>ROLLOFF_35</constant></entry>
	    <entry align="char">Roloff factor: &alpha;=35%</entry>
	</row><row>
	    <entry align="char" id="ROLLOFF-20"><constant>ROLLOFF_20</constant></entry>
	    <entry align="char">Roloff factor: &alpha;=20%</entry>
	</row><row>
	    <entry align="char" id="ROLLOFF-25"><constant>ROLLOFF_25</constant></entry>
	    <entry align="char">Roloff factor: &alpha;=25%</entry>
	</row><row>
	    <entry align="char" id="ROLLOFF-AUTO"><constant>ROLLOFF_AUTO</constant></entry>
	    <entry align="char">Auto-detect the roloff factor.</entry>
	</row>
        </tbody>
    </tgroup>
</table>
		</section>
	</section>
	<section id="DTV-DISEQC-SLAVE-REPLY">
	<title><constant>DTV_DISEQC_SLAVE_REPLY</constant></title>
	<para>Currently not implemented.</para>
	</section>
	<section id="DTV-FE-CAPABILITY-COUNT">
	<title><constant>DTV_FE_CAPABILITY_COUNT</constant></title>
	<para>Currently not implemented.</para>
	</section>
	<section id="DTV-FE-CAPABILITY">
	<title><constant>DTV_FE_CAPABILITY</constant></title>
	<para>Currently not implemented.</para>
	</section>
	<section id="DTV-DELIVERY-SYSTEM">
		<title><constant>DTV_DELIVERY_SYSTEM</constant></title>
		<para>Specifies the type of Delivery system</para>
		<section id="fe-delivery-system-t">
		<title>fe_delivery_system type</title>
		<para>Possible values: </para>

<table pgwide="1" frame="none" id="fe-delivery-system">
    <title>enum fe_delivery_system</title>
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
		<entry id="SYS-UNDEFINED"><constant>SYS_UNDEFINED</constant></entry>
		<entry>Undefined standard. Generally, indicates an error</entry>
	</row><row>
		<entry id="SYS-DVBC-ANNEX-A"><constant>SYS_DVBC_ANNEX_A</constant></entry>
		<entry>Cable TV: DVB-C following ITU-T J.83 Annex A spec</entry>
	</row><row>
		<entry id="SYS-DVBC-ANNEX-B"><constant>SYS_DVBC_ANNEX_B</constant></entry>
		<entry>Cable TV: DVB-C following ITU-T J.83 Annex B spec (ClearQAM)</entry>
	</row><row>
		<entry id="SYS-DVBC-ANNEX-C"><constant>SYS_DVBC_ANNEX_C</constant></entry>
		<entry>Cable TV: DVB-C following ITU-T J.83 Annex C spec</entry>
	</row><row>
		<entry id="SYS-ISDBC"><constant>SYS_ISDBC</constant></entry>
		<entry>Cable TV: ISDB-C (no drivers yet)</entry>
	</row><row>
		<entry id="SYS-DVBT"><constant>SYS_DVBT</constant></entry>
		<entry>Terrestral TV: DVB-T</entry>
	</row><row>
		<entry id="SYS-DVBT2"><constant>SYS_DVBT2</constant></entry>
		<entry>Terrestral TV: DVB-T2</entry>
	</row><row>
		<entry id="SYS-ISDBT"><constant>SYS_ISDBT</constant></entry>
		<entry>Terrestral TV: ISDB-T</entry>
	</row><row>
		<entry id="SYS-ATSC"><constant>SYS_ATSC</constant></entry>
		<entry>Terrestral TV: ATSC</entry>
	</row><row>
		<entry id="SYS-ATSCMH"><constant>SYS_ATSCMH</constant></entry>
		<entry>Terrestral TV (mobile): ATSC-M/H</entry>
	</row><row>
		<entry id="SYS-DTMB"><constant>SYS_DTMB</constant></entry>
		<entry>Terrestrial TV: DTMB</entry>
	</row><row>
		<entry id="SYS-DVBS"><constant>SYS_DVBS</constant></entry>
		<entry>Satellite TV: DVB-S</entry>
	</row><row>
		<entry id="SYS-DVBS2"><constant>SYS_DVBS2</constant></entry>
		<entry>Satellite TV: DVB-S2</entry>
	</row><row>
		<entry id="SYS-TURBO"><constant>SYS_TURBO</constant></entry>
		<entry>Satellite TV: DVB-S Turbo</entry>
	</row><row>
		<entry id="SYS-ISDBS"><constant>SYS_ISDBS</constant></entry>
		<entry>Satellite TV: ISDB-S</entry>
	</row><row>
		<entry id="SYS-DAB"><constant>SYS_DAB</constant></entry>
		<entry>Digital audio: DAB (not fully supported)</entry>
	</row><row>
		<entry id="SYS-DSS"><constant>SYS_DSS</constant></entry>
		<entry>Satellite TV:"DSS (not fully supported)</entry>
	</row><row>
		<entry id="SYS-CMMB"><constant>SYS_CMMB</constant></entry>
		<entry>Terrestral TV (mobile):CMMB (not fully supported)</entry>
	</row><row>
		<entry id="SYS-DVBH"><constant>SYS_DVBH</constant></entry>
		<entry>Terrestral TV (mobile): DVB-H (standard deprecated)</entry>
	</row>
        </tbody>
    </tgroup>
</table>


</section>
	</section>
	<section id="DTV-ISDBT-PARTIAL-RECEPTION">
		<title><constant>DTV_ISDBT_PARTIAL_RECEPTION</constant></title>

		<para>If <constant>DTV_ISDBT_SOUND_BROADCASTING</constant> is '0' this bit-field represents whether
			the channel is in partial reception mode or not.</para>

		<para>If '1' <constant>DTV_ISDBT_LAYERA_*</constant> values are assigned to the center segment and
			<constant>DTV_ISDBT_LAYERA_SEGMENT_COUNT</constant> has to be '1'.</para>

		<para>If in addition <constant>DTV_ISDBT_SOUND_BROADCASTING</constant> is '1'
			<constant>DTV_ISDBT_PARTIAL_RECEPTION</constant> represents whether this ISDB-Tsb channel
			is consisting of one segment and layer or three segments and two layers.</para>

		<para>Possible values: 0, 1, -1 (AUTO)</para>
	</section>
	<section id="DTV-ISDBT-SOUND-BROADCASTING">
		<title><constant>DTV_ISDBT_SOUND_BROADCASTING</constant></title>

		<para>This field represents whether the other DTV_ISDBT_*-parameters are
			referring to an ISDB-T and an ISDB-Tsb channel. (See also
			<constant>DTV_ISDBT_PARTIAL_RECEPTION</constant>).</para>

		<para>Possible values: 0, 1, -1 (AUTO)</para>
	</section>
	<section id="DTV-ISDBT-SB-SUBCHANNEL-ID">
		<title><constant>DTV_ISDBT_SB_SUBCHANNEL_ID</constant></title>

		<para>This field only applies if <constant>DTV_ISDBT_SOUND_BROADCASTING</constant> is '1'.</para>

		<para>(Note of the author: This might not be the correct description of the
			<constant>SUBCHANNEL-ID</constant> in all details, but it is my understanding of the technical
			background needed to program a device)</para>

		<para>An ISDB-Tsb channel (1 or 3 segments) can be broadcasted alone or in a
			set of connected ISDB-Tsb channels. In this set of channels every
			channel can be received independently. The number of connected
			ISDB-Tsb segment can vary, e.g. depending on the frequency spectrum
			bandwidth available.</para>

		<para>Example: Assume 8 ISDB-Tsb connected segments are broadcasted. The
			broadcaster has several possibilities to put those channels in the
			air: Assuming a normal 13-segment ISDB-T spectrum he can align the 8
			segments from position 1-8 to 5-13 or anything in between.</para>

		<para>The underlying layer of segments are subchannels: each segment is
			consisting of several subchannels with a predefined IDs. A sub-channel
			is used to help the demodulator to synchronize on the channel.</para>

		<para>An ISDB-T channel is always centered over all sub-channels. As for
			the example above, in ISDB-Tsb it is no longer as simple as that.</para>

		<para><constant>The DTV_ISDBT_SB_SUBCHANNEL_ID</constant> parameter is used to give the
			sub-channel ID of the segment to be demodulated.</para>

		<para>Possible values: 0 .. 41, -1 (AUTO)</para>
	</section>
	<section id="DTV-ISDBT-SB-SEGMENT-IDX">
		<title><constant>DTV_ISDBT_SB_SEGMENT_IDX</constant></titl