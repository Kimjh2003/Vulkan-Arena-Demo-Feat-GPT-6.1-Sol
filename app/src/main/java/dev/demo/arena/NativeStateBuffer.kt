package dev.demo.arena

import java.nio.ByteBuffer
import java.nio.ByteOrder

/** One native-order wire buffer per render session; never shared with UI callbacks. */
internal class NativeStateBuffer {
    val bytes: ByteBuffer=ByteBuffer.allocateDirect(HudState.WIRE_BYTES).order(ByteOrder.nativeOrder())
    private val published=IntArray(13)
    private var initialized=false
    val events: Int get()=bytes.getInt(52)
    val terminal: Boolean get()=bytes.getInt(32)!=0 || bytes.getInt(36)!=0
    val upgrading: Boolean get()=bytes.getInt(44)!=0
    fun changedSnapshot(): HudState? {
        var changed=!initialized
        for(i in published.indices) if(published[i]!=bytes.getInt(i*4)) changed=true
        if(!changed) return null
        for(i in published.indices) published[i]=bytes.getInt(i*4)
        initialized=true
        return HudState.read(bytes)
    }
}
