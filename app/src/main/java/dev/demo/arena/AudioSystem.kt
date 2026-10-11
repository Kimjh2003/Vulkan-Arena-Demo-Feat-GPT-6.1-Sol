package dev.demo.arena

import android.content.Context
import android.media.AudioAttributes
import android.media.SoundPool
import java.util.concurrent.ConcurrentHashMap

/** Small, locally synthesized effects. Called from the render thread. */
internal class AudioSystem(context: Context) {
    private val ready = ConcurrentHashMap.newKeySet<Int>()
    private val pool = SoundPool.Builder().setMaxStreams(8)
        .setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build()).build()
    @Volatile var enabled = true
    @Volatile private var active = true
    private val shot: Int
    private val impact: Int
    private val hurt: Int
    init {
        pool.setOnLoadCompleteListener { _, id, status -> if (status == 0) ready.add(id) }
        shot = pool.load(context, R.raw.shot, 1)
        impact = pool.load(context, R.raw.impact, 1)
        hurt = pool.load(context, R.raw.hurt, 1)
    }
    fun playEvents(events: Int) {
        if (!enabled || !active) return
        fun play(id: Int, volume: Float, priority: Int) {
            if (ready.contains(id)) pool.play(id, volume, volume, priority, 0, 1f)
        }
        if (events and 1 != 0) play(shot, .38f, 1)
        if (events and 2 != 0) play(impact, .28f, 1)
        if (events and 4 != 0) play(hurt, .55f, 2)
    }
    fun pause() { active = false; pool.autoPause() }
    fun resume() { active = true; pool.autoResume() }
    fun release() { active = false; pool.setOnLoadCompleteListener(null); pool.release(); ready.clear() }
}
