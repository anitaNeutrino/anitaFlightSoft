#ifndef UNWRAP_H
#define UNWRAP_H

int readWrapOffsets(int offsetOption);

int unwrapTransient(AnitaEventBody_t *rawSurfEvent,
		    AnitaTransientBody8_t SurfTransientPS, 
		    AnitaTransientBody8_t * surfTransientUW);

#endif /* UNWRAP_H */
