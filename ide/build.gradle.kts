plugins {
    kotlin("multiplatform") version "1.9.21"
    kotlin("plugin.serialization") version "1.9.21"
    id("com.github.johnrengelman.shadow") version "8.1.1"
    application
}

group = "com.foundry"
version = "1.0.0"

repositories {
    mavenCentral()
    maven("https://maven.pkg.jetbrains.space/public/p/compose/dev")
}

kotlin {
    jvm {
        compilations.all {
            kotlinOptions.jvmTarget = "17"
        }
        withJava()
        testRuns.named("test").configure {
            executionTask.configure {
                useJUnitPlatform()
            }
        }
    }

    js(IR) {
        browser {
            commonWebpackConfig {
                cssSupport {
                    enabled.set(true)
                }
            }
            webpackTask {
                cssSupport {
                    enabled.set(true)
                }
            }
            runTask {
                cssSupport {
                    enabled.set(true)
                }
            }
            testTask {
                useKarma {
                    useChromeHeadless()
                    webpackConfig.cssSupport {
                        enabled.set(true)
                    }
                }
            }
        }
        binaries.executable()
    }

    sourceSets {
        val commonMain by getting {
            dependencies {
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.2")
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3")
                implementation("org.jetbrains.kotlinx:kotlinx-browser:0.1")
            }
        }

        val commonTest by getting {
            dependencies {
                implementation(kotlin("test"))
            }
        }

        val jvmMain by getting {
            dependencies {
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-swing:1.7.3")
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.2")
                implementation("org.jetbrains.compose.ui:ui:1.5.11")
                implementation("org.jetbrains.compose.ui:ui-tooling:1.5.11")
                implementation("org.jetbrains.compose.foundation:foundation:1.5.11")
                implementation("org.jetbrains.compose.material:material:1.5.11")
                implementation("org.jetbrains.compose.runtime:runtime:1.5.11")
                implementation("org.jetbrains.compose.desktop:desktop:1.5.11")
                implementation("org.jetbrains.compose.animation:animation:1.5.11")
                implementation("io.ktor:ktor-client-core:2.3.7")
                implementation("io.ktor:ktor-client-cio:2.3.7")
                implementation("io.ktor:ktor-client-websockets:2.3.7")
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.2")
            }
        }

        val jvmTest by getting {
            dependencies {
                implementation("org.jetbrains.kotlin:kotlin-test-junit5")
                implementation("org.junit.jupiter:junit-jupiter-engine:5.9.2")
            }
        }

        val jsMain by getting {
            dependencies {
                implementation("org.jetbrains.kotlinx:kotlinx-html-js:0.8.1")
                implementation("org.jetbrains.kotlinx:kotlinx-serialization-json-js:1.6.2")
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core-js:1.7.3")
                implementation("org.jetbrains.compose.web:web:1.5.11")
                implementation("org.jetbrains.compose.runtime:runtime:1.5.11")
                implementation("io.ktor:ktor-client-js:2.3.7")
            }
        }

        val jsTest by getting {
            dependencies {
                implementation(kotlin("test-js"))
            }
        }
    }
}

application {
    mainClass.set("com.foundry.ide.MainKt")
}

tasks.named<JavaExec>("run") {
    dependsOn(tasks.named("jvmProcessResources"))
    systemProperty("compose.application.configure.swing.globals", "true")
    systemProperty("compose.interop.blending", "true")
}

tasks.shadowJar {
    manifest {
        attributes(
            "Main-Class" to "com.foundry.ide.MainKt",
            "Implementation-Title" to "Foundry IDE",
            "Implementation-Version" to version
        )
    }
}

tasks.register<Jar>("jvmSourcesJar") {
    archiveClassifier.set("sources")
    from(sourceSets["jvmMain"].allSource)
}

tasks.register<Jar>("jsSourcesJar") {
    archiveClassifier.set("sources")
    from(sourceSets["jsMain"].allSource)
}
