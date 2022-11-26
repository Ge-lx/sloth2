#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

class VisualizationHandler {

private:
	SDL_mutex* vh_mutex;
    SDL_cond* vh_cond;
    SDL_Thread* vh_thread;

    bool should_stop = false;
    bool buffer_processed = false;
    double const* buffer;

    static int worker_thread (void * _self) {
    	VisualizationHandler* self = static_cast<VisualizationHandler*>(_self);
    	while (true) {
    		SDL_LockMutex(self->vh_mutex);
			if (self->should_stop) {
				SDL_UnlockMutex(self->vh_mutex);
				SDL_CondBroadcast(self->vh_cond);
				printf("Exiting worker_thread.\n");
				break;
			}

			SDL_CondWait(self->vh_cond, self->vh_mutex);
			if (self->buffer_processed == true) {
				continue;
			}

			self->visualize(self->buffer);
			self->buffer_processed = true;

	        SDL_UnlockMutex(self->vh_mutex);
	        SDL_CondSignal(self->vh_cond);
    	}
    	return 0;
	}

	void stop_thread () {
		// Send stop signal
		SDL_LockMutex(vh_mutex);
		should_stop = true;
		SDL_UnlockMutex(vh_mutex);
		SDL_CondSignal(vh_cond);

		// Await thread exit
		SDL_WaitThread(vh_thread, NULL);
	}

	virtual void visualize (double const* data) = 0;
	virtual void render (SDL_Renderer* renderer) = 0;

protected:
    SDL_AudioSpec const& spec;

public:

	virtual void process_ring_buffer (double const* data) final {
		SDL_LockMutex(vh_mutex);

		buffer = data;
		buffer_processed = false;

        SDL_UnlockMutex(vh_mutex);
        SDL_CondSignal(vh_cond);
	}

	void await_buffer_processed () {
		SDL_LockMutex(vh_mutex);
		while (buffer_processed == false) {
			SDL_CondWait(vh_cond, vh_mutex);
		}
		SDL_UnlockMutex(vh_mutex);
	}

	void await_and_render (SDL_Renderer* renderer) {
		await_buffer_processed();
		SDL_LockMutex(vh_mutex);
		render(renderer);
		SDL_UnlockMutex(vh_mutex);
	}

	VisualizationHandler (SDL_AudioSpec const& spec) : spec(spec) {
		vh_mutex = SDL_CreateMutex();
        vh_cond = SDL_CreateCond();
		vh_thread = SDL_CreateThread(&VisualizationHandler::worker_thread, "visualization worker", (void *) this);
	}

    ~VisualizationHandler () {
    	stop_thread();
        SDL_DestroyCond(vh_cond);
        SDL_DestroyMutex(vh_mutex);
    }
};
