pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
    plugins {
        // connect-client 1.1.0 verlangt AGP 8.9.1 oder neuer - der Bau bricht
        // sonst schon beim Pruefen der AAR-Metadaten ab.
        id("com.android.application") version "8.13.0"
        id("org.jetbrains.kotlin.android") version "2.1.20"
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "herzintervall-companion"
include(":app")
