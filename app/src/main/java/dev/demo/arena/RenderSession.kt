package dev.demo.arena

import android.os.Handler
import android.os.Looper
import android.view.SurfaceView
import java.util.concurrent.Semaphore

/** Single worker and bounded UI publication. Start/stop/refresh are UI-thread operations. */
internal class RenderSession(
    private val surface: SurfaceView,
    private val input: TouchInput,
    private val actions: RoundActions,
    private val audio: AudioSystem,
    private val manual: Boolean,
    private val weapon: Int,
    private val textureFamily: Int,
    onState: (HudState)->Unit,
    private val onError: (String)->Unit
) : Runnable {
    private val clock=FrameClock(DisplayProfileSource(surface))
    private val mailbox=StateMailbox(onState)
    private val ui=Handler(Looper.getMainLooper())
    private val completed=Semaphore(0)
    private var worker: Thread?=null
    @Volatile var isRunning=false
        private set
    @Volatile private var closed=false

    fun start() {
        check(worker==null && !closed)
        isRunning=true;clock.start()
        worker=Thread(this,"ArenaRenderer").also { it.start() }
    }
    fun refreshDisplay() = clock.refreshDisplay()
    private fun error(message: String) {
        ui.post { if(!closed) onError(message) }
    }
    override fun run() {
        try {
            val failure=NativeBridge.start(surface.holder.surface,manual,weapon,textureFamily)
            if(failure.isNotEmpty()) {error(failure);return}
            val state=NativeStateBuffer()
            var last=System.nanoTime()
            var count=0
            var terminalPosted=false
            var upgradeShown=false
            while(isRunning) {
                if(!clock.awaitFrame()) continue
                if(!isRunning) break
                val prepared=NativeBridge.prepare()
                if(prepared==1) continue
                if(prepared<0) {error("Rendering surface unavailable. Back to retry.");break}
                if(!isRunning) break
                val now=System.nanoTime()
                val dt=(now-last)/1e9f;last=now
                val controls=input.current
                val response=NativeBridge.frame(dt,controls.moveX,controls.moveZ,controls.aimX,controls.aimZ,
                    controls.shooting,actions.takeDash(),actions.takeUpgrade(),state.bytes)
                if(response<0) {error("Game interrupted. Back to retry.");break}
                audio.playEvents(state.events)
                val terminal=state.terminal
                val upgrading=state.upgrading
                if(!terminalPosted && (++count%10==0 || terminal || upgrading!=upgradeShown)) {
                    terminalPosted=terminal;upgradeShown=upgrading
                    val snapshot = state.changedSnapshot()
                    if (snapshot != null) mailbox.offer(snapshot)
                }
            }
        } finally {
            try { NativeBridge.stop() }
            finally {
                isRunning=false
                ui.post { clock.stop() }
                completed.release()
            }
        }
    }
    fun stop() {
        closed=true;isRunning=false;clock.stop();mailbox.close()
        if(worker!=null) completed.acquireUninterruptibly()
        worker=null
    }
}
