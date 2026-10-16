package dev.demo.arena

import android.hardware.display.DisplayManager
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View

/** System-provided window/display metadata. Surface capabilities remain authoritative for Vulkan. */
internal data class DisplayProfile(
    val displayId: Int, val modeId: Int, val refreshHz: Float,
    val rotation: Int, val surfaceWidth: Int, val surfaceHeight: Int,
    val wideColor: Boolean, val state: Int
)

/** Owned by a live render surface; callbacks and capture run on the UI thread. */
internal class DisplayProfileSource(private val view: View) : DisplayManager.DisplayListener {
    private val manager=view.context.getSystemService(DisplayManager::class.java)
    private var registered=false
    var current: DisplayProfile? = null
        private set

    fun start() {
        if(!registered) {
            manager.registerDisplayListener(this, Handler(Looper.getMainLooper()))
            registered=true
        }
        capture()
    }

    fun capture() {
        val display=view.display
        val next=display?.let {
            DisplayProfile(it.displayId,it.mode.modeId,it.refreshRate,it.rotation,
                view.width,view.height,it.isWideColorGamut,it.state)
        }
        if(next!=current) {
            current=next
            Log.i("ArenaDisplay","System profile: $next")
        }
    }

    override fun onDisplayChanged(displayId: Int) {
        if(displayId==current?.displayId || displayId==view.display?.displayId) capture()
    }
    override fun onDisplayAdded(displayId: Int) = capture()
    override fun onDisplayRemoved(displayId: Int) = capture()

    fun stop() {
        if(registered) manager.unregisterDisplayListener(this)
        registered=false
    }
}
