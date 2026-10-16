package dev.demo.arena

/** Debug instrumentation only; excluded from benchmark/release/profile source sets. */
internal object NativeDiagnostics {
    init { System.loadLibrary("arena") }
    external fun renderedFrames(): Long
    external fun rendererGeneration(): Long
}
